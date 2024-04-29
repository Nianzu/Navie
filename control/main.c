// g++ -o test main.c Realtime.cc -lm -lpigpio

#include <pigpio.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <math.h>

// Define GPIO pins according to Broadcom numbering
#define MOTOR_L_R 25 // GPIO25, equivalent to physical pin 22
#define MOTOR_L_F 8  // GPIO8, equivalent to physical pin 24
#define MOTOR_R_F 7  // GPIO7, equivalent to physical pin 26
#define MOTOR_R_R 24 // GPIO24, equivalent to physical pin 18

#define LIGHT_L 2 // GPIO2, equivalent to physical pin 3
#define LIGHT_R 3 // GPIO3, equivalent to physical pin 5

#define ENCODER_L_A 10 // GPIO10, equivalent to physical pin 19
#define ENCODER_L_B 9  // GPIO9, equivalent to physical pin 21
#define ENCODER_R_A 17 // GPIO17, equivalent to physical pin 11
#define ENCODER_R_B 27 // GPIO27, equivalent to physical pin 13

#define TICKS_PER_REV 3575.0855
#define TICKS_PER_RAD 568.99252930116018242497
#define WHEEL_CIRCUMFERENCE 0.147
#define MAX_SPEED 255

struct termios orig_term;

void restore_terminal()
{
    // Restore the original terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);
}

void setup_terminal() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    orig_term = term;
    // term.c_lflag &= ~(ICANON | ECHO);
    term.c_lflag &= ~(ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

char read_keyboard_input()
{
    char c;
    read(STDIN_FILENO, &c, 1);
    return c;
}

#define STATE_MASK 0x03 // Mask to extract the last two bits (encoder states)
#define CW 1            // Clockwise rotation identifier
#define CCW -1          // Counter-clockwise rotation identifier

int8_t state_table[] = {
    0, // 00 to 00: No change
    CW, // 00 to 01: Clockwise step
    CCW, // 00 to 10: Counter-clockwise step
    0, // 00 to 11: Invalid transition
    CCW, // 01 to 00: Counter-clockwise step
    0, // 01 to 01: No change
    -2, // 01 to 10: Invalid transition
    CW, // 01 to 11: Clockwise step
    CW, // 10 to 00: Clockwise step
    2, // 10 to 01: Invalid transition
    0, // 10 to 10: No change
    CCW, // 10 to 11: Counter-clockwise step
    0, // 11 to 00: Invalid transition
    CCW, // 11 to 01: Counter-clockwise step
    CW, // 11 to 10: Clockwise step
    0 // 11 to 11: No change
};

int update_encoder(uint8_t *encoder_l_a, uint8_t *encoder_l_b, uint8_t encoder_l_a_new, uint8_t encoder_l_b_new) {
    uint8_t old_state = (*encoder_l_a << 1) | *encoder_l_b;
    uint8_t new_state = (encoder_l_a_new << 1) | encoder_l_b_new;
    int output = state_table[(old_state << 2) | new_state]; // Shift old state 2 bits left and add new state
    // if (output >1)
    // {
    //     printf("ERROR %d\n",output);
    // }
    *encoder_l_a = encoder_l_a_new;
    *encoder_l_b = encoder_l_b_new;

    return output;
}

enum motion {
  fwd,
  rev,
  stop
}; 

enum motors {
    left,
    right
};

void set_motor(motors motor_sel, motion motion_sel, int speed)
{
    gpioPWM(motor_sel == left ? 12 : 13, speed);
    switch (motion_sel)
    {
        case fwd:
            gpioWrite(motor_sel == left ? MOTOR_L_F : MOTOR_R_F, PI_HIGH);
            gpioWrite(motor_sel == left ? MOTOR_L_R : MOTOR_R_R, PI_LOW);
            break;
        case rev:
            gpioWrite(motor_sel == left ? MOTOR_L_F : MOTOR_R_F, PI_LOW);
            gpioWrite(motor_sel == left ? MOTOR_L_R : MOTOR_R_R, PI_HIGH);
            break;
        default:
        case stop:
            gpioWrite(motor_sel == left ? MOTOR_L_F : MOTOR_R_F, PI_LOW);
            gpioWrite(motor_sel == left ? MOTOR_L_R : MOTOR_R_R, PI_LOW);
            break;
    }
}

typedef struct {
    double Kp;
    double Ki;
    double Kd;
    int last_error;
    double integral;
} PIDController;

void PID_Init(PIDController* pid, double Kp, double Ki, double Kd) {
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    pid->last_error = 0;
    pid->integral = 0.0;
}

int PID_Update(PIDController* pid, int setpoint, int actual) {
    int error = setpoint - actual;
    pid->integral += error;
    double derivative = error - pid->last_error;
    int output = (int)(pid->Kp * error + pid->Ki * pid->integral + pid->Kd * derivative);
    pid->last_error = error;
    return output;
}

void drive_distance_in_time(double distance_meters, double total_time_seconds, bool flip_left) {
    double ticks_per_meter = TICKS_PER_REV / WHEEL_CIRCUMFERENCE;
    int total_ticks = (int)(distance_meters * ticks_per_meter);

    // Time allocation (in seconds, for clarity)
    double accel_time = total_time_seconds / 3;
    double decel_time = accel_time; 
    double constant_speed_time = total_time_seconds - 2 * accel_time;
    
    // Speed calculations (in ticks per second for simplicity)
    int max_speed_ticks_per_second = total_ticks / (accel_time + constant_speed_time);
    
    PIDController pid_left, pid_right;
    PID_Init(&pid_left, 1, 0.000001, 0.01);
    PID_Init(&pid_right, 1, 0.000001, 0.01);
    // PID_Init(&pid_left, 0.1, 0.00, 0.05);
    // PID_Init(&pid_right, 0.1, 0.00, 0.05);

    struct timespec start, current;
    clock_gettime(CLOCK_MONOTONIC, &start);
    long long current_time_ns = 0;
    double current_time_seconds = 0;
    int current_left_ticks = 0, current_right_ticks = 0;
    int target_ticks = 0;

    uint8_t encoder_l_a = gpioRead(ENCODER_L_A);
    uint8_t encoder_l_b = gpioRead(ENCODER_L_B);
    uint8_t encoder_r_a = gpioRead(ENCODER_R_A);
    uint8_t encoder_r_b = gpioRead(ENCODER_R_B);
    double right_scale = 1.5;

    while (abs(current_left_ticks - (flip_left ? -total_ticks : total_ticks)) > abs(total_ticks / 40) || abs(current_right_ticks/right_scale - total_ticks) > abs(total_ticks / 40)) {
        clock_gettime(CLOCK_MONOTONIC, &current);
        current_time_ns = (current.tv_sec - start.tv_sec) * 1000000000LL + (current.tv_nsec - start.tv_nsec);
        current_time_seconds = current_time_ns / 1e9; // Convert nanoseconds to seconds for calculations

        // Calculate target_ticks based on trapezoidal profile
        if (current_time_seconds <= accel_time) 
        {
            target_ticks = (max_speed_ticks_per_second * current_time_seconds / accel_time) * current_time_seconds / 2;
        } else if (current_time_seconds <= (accel_time + constant_speed_time)) {
            target_ticks = max_speed_ticks_per_second * (current_time_seconds - accel_time / 2);
        } else if (current_time_seconds < total_time_seconds) {
            double t_dec = current_time_seconds - accel_time - constant_speed_time;
            target_ticks = (max_speed_ticks_per_second * (accel_time + constant_speed_time / 2)) + max_speed_ticks_per_second * t_dec - (max_speed_ticks_per_second * t_dec / decel_time) * t_dec / 2;
        } else {
            target_ticks = total_ticks;
        }

        // printf("%f %d %d %d\n",current_time_seconds,target_ticks,current_left_ticks,current_right_ticks);

        // Update PID controllers for each motor
        int left_speed = PID_Update(&pid_left, flip_left ? -target_ticks : target_ticks, current_left_ticks);
        int right_speed = PID_Update(&pid_right, target_ticks, current_right_ticks/right_scale);

        // Clamp speeds to [-MAX_SPEED, MAX_SPEED]
        left_speed = fmax(-MAX_SPEED, fmin(left_speed, MAX_SPEED));
        right_speed = fmax(-MAX_SPEED, fmin(right_speed, MAX_SPEED));
        // printf("\t%d\n",left_speed);
        set_motor(left, left_speed > 0 ? fwd : rev, abs(left_speed));
        set_motor(right, right_speed > 0 ? fwd : rev, abs(right_speed));

        // Update current encoder counts
        uint8_t encoder_l_a_new = gpioRead(ENCODER_L_A);
        uint8_t encoder_l_b_new = gpioRead(ENCODER_L_B);
        uint8_t encoder_r_a_new = gpioRead(ENCODER_R_A);
        uint8_t encoder_r_b_new = gpioRead(ENCODER_R_B);
        current_left_ticks -= update_encoder(&encoder_l_a, &encoder_l_b, encoder_l_a_new, encoder_l_b_new);
        current_right_ticks += update_encoder(&encoder_r_a, &encoder_r_b, encoder_r_a_new, encoder_r_b_new);
    }

    // Stop motors after reaching the target
    set_motor(left, stop, 0);
    set_motor(right, stop, 0);
}

void turn_right()
{
    drive_distance_in_time(-0.043,2,true);
}

void turn_left()
{
    drive_distance_in_time(0.043,2,true);
}

int main(int argc, char **argv)
{
    // Set the terminal to take inputs immediately, without waiting for [ENTER]
    setup_terminal();
    if (gpioInitialise() < 0)
        return 1;

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    // Configure the two PWM ports that control the speed of the wheels
    // Left
    gpioSetMode(12,PI_ALT0);
    gpioSetPWMfrequency(12, 1000); // PWM0

    // Right
    gpioSetMode(13,PI_ALT0);
    gpioSetPWMfrequency(13, 1000); // PWM0

    // Set the motors as outputs
    gpioSetMode(MOTOR_L_R, PI_OUTPUT);
    gpioSetMode(MOTOR_L_F, PI_OUTPUT);
    gpioSetMode(MOTOR_R_F, PI_OUTPUT);
    gpioSetMode(MOTOR_R_R, PI_OUTPUT);

    // Set the lights as outputs
    gpioSetMode(LIGHT_L, PI_OUTPUT);
    gpioSetMode(LIGHT_R, PI_OUTPUT);

    gpioWrite(LIGHT_L, PI_LOW);
    gpioWrite(LIGHT_R, PI_LOW);

    // Set the encoders as inputs
    gpioSetMode(ENCODER_L_A, PI_INPUT);
    gpioSetMode(ENCODER_L_B, PI_INPUT);
    
    // Init the encoders
    uint8_t encoder_l_a = gpioRead(ENCODER_L_A);
    uint8_t encoder_l_b = gpioRead(ENCODER_L_B);
    uint8_t encoder_r_a = gpioRead(ENCODER_R_A);
    uint8_t encoder_r_b = gpioRead(ENCODER_R_B);


    int c;
    int num_ticks = 0;
    while (1)
    {
        c = read_keyboard_input();
        if (c == 'w')
        {
            // Move the robot forward
            printf("For\n");
            set_motor(right, fwd, 255);
            set_motor(left, fwd, 255);
        }
        else if (c == 'a')
        {
            // Turn the robot left
            printf("L\n");
            set_motor(right, fwd, 100);
            set_motor(left, rev, 100);
        }
        else if (c == 's')
        {
            // Move the robot backward
            printf("Rev\n");
            set_motor(right, rev, 255);
            set_motor(left, rev, 255);
        }
        else if (c == 'd')
        {
            // Turn the robot right
            printf("R\n");
            set_motor(right, rev, 100);
            set_motor(left, fwd, 100);
        }
        else if (c == ' ')
        {
            // Stop the robot
            printf("STOP\n");
            set_motor(right, stop, 0);
            set_motor(left, stop, 0);
        }
        else if (c == 'p')
        {
            // Print some information
            printf("%d\n",num_ticks);
        }
        else if (c == 'k')
        {
            drive_distance_in_time(0.4,10,false);
        }
        else if (c == 'm')
        {
            drive_distance_in_time(-0.4,10,false);
        }
        else if (c == 'l')
        {
            turn_right();
        }
        else if (c == 'j')
        {
            turn_left();
        }
        else if (c == 'r')
        {
            drive_distance_in_time(0.2,5,false);
            turn_left();
            drive_distance_in_time(0.2,5,false);
            turn_left();
            drive_distance_in_time(0.2,5,false);
	    turn_right();
            drive_distance_in_time(0.6,12,false);
	    turn_right();
            drive_distance_in_time(0.4,8,false);
	    turn_right();
            drive_distance_in_time(0.2,5,false);
        }
        else if (c == 'e')
        {
            break;
        }

        // CW is positive ticks
        uint8_t encoder_l_a_new = gpioRead(ENCODER_L_A);
        uint8_t encoder_l_b_new = gpioRead(ENCODER_L_B);
        uint8_t encoder_r_a_new = gpioRead(ENCODER_R_A);
        uint8_t encoder_r_b_new = gpioRead(ENCODER_R_B);
        // left encoder update
        // num_ticks += update_encoder(&encoder_l_a, &encoder_l_b, encoder_l_a_new, encoder_l_b_new);
        num_ticks += update_encoder(&encoder_r_a, &encoder_r_b, encoder_r_a_new, encoder_r_b_new);
    }
    restore_terminal();
    gpioTerminate();
    return 0;
}
