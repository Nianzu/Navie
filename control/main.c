// gcc -o test test.c -lm -lbcm2835

#include <bcm2835.h>
#include <stdio.h>
#include <ncurses.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#define MOTOR_L_R RPI_V2_GPIO_P1_22
#define MOTOR_L_F RPI_V2_GPIO_P1_24
#define MOTOR_R_F RPI_V2_GPIO_P1_26
#define MOTOR_R_R RPI_V2_GPIO_P1_18

#define LIGHT_L RPI_V2_GPIO_P1_03
#define LIGHT_R RPI_V2_GPIO_P1_05

#define ENCODER_R_A RPI_V2_GPIO_P1_19
#define ENCODER_R_B RPI_V2_GPIO_P1_21
#define ENCODER_L_A RPI_V2_GPIO_P1_11
#define ENCODER_L_B RPI_V2_GPIO_P1_13

#define TICKS_PER_REV 3575.0855;
#define TICKS_PER_RAD 568.99252930116018242497;


void setup_terminal()
{
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

char read_keyboard_input()
{
    char c;
    read(STDIN_FILENO, &c, 1);
    return c;
}

void interrupt_handler_la()
{
    printf("LA - ");
        if (bcm2835_gpio_lev(ENCODER_L_A))
    {
        // Handle rising edge here
        printf("R\n");
    }
    else
    {
        // Handle falling edge here
        printf("F\n");
    }
}

int main(int argc, char **argv)
{
    // If you call this, it will not actually access the GPIO
    // Use for testing
    // bcm2835_set_debug(1);

    // Set the terminal to take inputs immediately, without waiting for [ENTER]
    setup_terminal();
    if (!bcm2835_init())
        return 1;

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    // Configure the two PWM ports that control the speed of the wheels
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_12, BCM2835_GPIO_FSEL_ALT5); // PWM0
    bcm2835_pwm_set_mode(0, 1, 1);                                // mark-space mode
    bcm2835_pwm_set_range(0, 1024);                               // 10-bit range
    bcm2835_pwm_set_clock(BCM2835_PWM_CLOCK_DIVIDER_16);          // 16 prescaler
    bcm2835_pwm_set_data(0, 1023);

    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_35, BCM2835_GPIO_FSEL_ALT5); // PWM0
    bcm2835_pwm_set_mode(1, 1, 1);                                // mark-space mode
    bcm2835_pwm_set_range(1, 1024);                               // 10-bit range
    bcm2835_pwm_set_clock(BCM2835_PWM_CLOCK_DIVIDER_16);          // 16 prescaler
    bcm2835_pwm_set_data(1, 1023);

    // Set the motors as outputs
    bcm2835_gpio_fsel(MOTOR_L_R, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(MOTOR_L_F, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(MOTOR_R_F, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(MOTOR_R_R, BCM2835_GPIO_FSEL_OUTP);

    // Set the lights as outputs
    bcm2835_gpio_fsel(LIGHT_L, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(LIGHT_R, BCM2835_GPIO_FSEL_OUTP);

    bcm2835_gpio_write(LIGHT_L, LOW);
    bcm2835_gpio_write(LIGHT_R, LOW);

    // Set the encoders as inputs
    bcm2835_gpio_fsel(ENCODER_L_A, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(ENCODER_L_B, BCM2835_GPIO_FSEL_INPT);
    
    // bcm2835_gpio_set_pud(ENCODER_L_A, BCM2835_GPIO_PUD_UP);
    // bcm2835_gpio_set_pud(ENCODER_L_B, BCM2835_GPIO_PUD_UP);
    // bcm2835_gpio_ren(ENCODER_L_A);
    // bcm2835_gpio_fen(ENCODER_L_A);
    // bcm2835_gpio_ren(ENCODER_L_B);
    // bcm2835_gpio_fen(ENCODER_L_B);

    uint8_t encoder_l_a = bcm2835_gpio_lev(ENCODER_L_A);
    uint8_t encoder_l_b = bcm2835_gpio_lev(ENCODER_L_B);
    int last_change = -1;
    int c;
    int num_ticks = 0;
    while (1)
    {
        c = read_keyboard_input();
        if (c == 'w')
        {
            // Move the robot forward
            printf("For\n");
            bcm2835_pwm_set_data(0, 1023);
            bcm2835_pwm_set_data(1, 1023);
            bcm2835_gpio_write(MOTOR_L_F, HIGH);
            bcm2835_gpio_write(MOTOR_L_R, LOW);
            bcm2835_gpio_write(MOTOR_R_F, HIGH);
            bcm2835_gpio_write(MOTOR_R_R, LOW);
        }
        else if (c == 'a')
        {
            // Turn the robot left
            printf("L\n");
            bcm2835_pwm_set_data(0, 500);
            bcm2835_pwm_set_data(1, 500);
            bcm2835_gpio_write(MOTOR_L_F, LOW);
            bcm2835_gpio_write(MOTOR_L_R, HIGH);
            bcm2835_gpio_write(MOTOR_R_F, HIGH);
            bcm2835_gpio_write(MOTOR_R_R, LOW);
        }
        else if (c == 's')
        {
            // Move the robot backward
            printf("Rev\n");
            bcm2835_pwm_set_data(0, 1023);
            bcm2835_pwm_set_data(1, 1023);
            bcm2835_gpio_write(MOTOR_L_F, LOW);
            bcm2835_gpio_write(MOTOR_L_R, HIGH);
            bcm2835_gpio_write(MOTOR_R_F, LOW);
            bcm2835_gpio_write(MOTOR_R_R, HIGH);
        }
        else if (c == 'd')
        {
            // Turn the robot right
            printf("R\n");
            bcm2835_pwm_set_data(0, 500);
            bcm2835_pwm_set_data(1, 500);
            bcm2835_gpio_write(MOTOR_L_F, HIGH);
            bcm2835_gpio_write(MOTOR_L_R, LOW);
            bcm2835_gpio_write(MOTOR_R_F, LOW);
            bcm2835_gpio_write(MOTOR_R_R, HIGH);
        }
        else if (c == ' ')
        {
            // Stop the robot
            printf("STOP\n");
            bcm2835_gpio_write(MOTOR_L_F, LOW);
            bcm2835_gpio_write(MOTOR_L_R, LOW);
            bcm2835_gpio_write(MOTOR_R_F, LOW);
            bcm2835_gpio_write(MOTOR_R_R, LOW);
        }
        else if (c == 'p')
        {
            // Print some information
            printf("%d\n",num_ticks);
        }
        else
        {
            // Invalid input
            // printf("Invalid input: %c\n", c);
        }

        // CW is positive ticks
        uint8_t encoder_l_a_new = bcm2835_gpio_lev(ENCODER_L_A);
        uint8_t encoder_l_b_new = bcm2835_gpio_lev(ENCODER_L_B);
        if(encoder_l_a_new != encoder_l_a)
        {
            if(encoder_l_a > encoder_l_a_new) // high to low
            {
                if(encoder_l_b)
                {
                    // printf("L\n");
                    num_ticks--;
                }
                else
                {
                    // printf("R\n");
                    num_ticks++;
                }
            }
            else
            {
                if(encoder_l_b)
                {
                    // printf("R\n");
                    num_ticks++;
                }
                else
                {
                    // printf("L\n");
                    num_ticks--;
                }
            }
            // printf("A\n");
            encoder_l_a = encoder_l_a_new;
            if(last_change == 1)
            {
                // num_ticks++;
            }
            last_change = 0;
            // printf("%d\n",num_ticks);
        }
        if(encoder_l_b_new != encoder_l_b)
        {
            if(encoder_l_b > encoder_l_b_new) // high to low
            {
                if(encoder_l_a)
                {
                    // printf("R\n");
                    num_ticks++;
                }
                else
                {
                    // printf("L\n");
                    num_ticks--;
                }
            }
            else
            {
                if(encoder_l_a)
                {
                    // printf("L\n");
                    num_ticks--;
                }
                else
                {
                    // printf("R\n");
                    num_ticks++;
                }
            }
            // printf("B\n");
            encoder_l_b = encoder_l_b_new;
            if(last_change == 0)
            {
                // num_ticks++;
            }
            last_change = 1;
            // printf("%d\n",num_ticks);
        }

    }
    bcm2835_close();
    return 0;
}