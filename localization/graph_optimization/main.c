// Nico Zucca, 1/2023

// Compile cmd:
// export LD_LIBRARY_PATH="/usr/local/lib"
// gcc main.c -o main.o `sdl2-config --cflags --libs` -lm -O3

#include <SDL2/SDL.h>
#include <math.h>
#include <limits.h>
#include <time.h>

// Height of the window
#define WINDOW_WIDTH 1000
// Width of the window
#define WINDOW_HEIGHT 1000
// Sensor offset for the side sensors in radians
#define SENSOR_OFFSET 0.75 //~90 deg
// Number of new nodes to create per second
#define MAX_NODE_FREQUENCY 1
// Number of sensors total
#define NUM_SENSORS 100

// Sensor vector
typedef struct sensor_vector
{
    double angle; // Rotation in radians
    double length;
    struct sensor_vector *next;
} sensor_vector;

// Agent object.
typedef struct agent
{
    double x;
    double y;
    double angle; // Rotation in radians
    struct sensor_vector *measurements;
} agent;

// Used to store anticipated movement
typedef struct movement
{
    double x;
    double y;
    double angular;

    double linear_total;
    double angular_total;
} movement;

// Path object
typedef struct connection
{
    double h;
    double x_1;
    double y_1;
    double x_2;
    double y_2;
    struct connection *next;
} connection;

// Graph node object
typedef struct graph_node
{
    // Connections
    double x, y, angular; // relative offset from last position
    double strength;      // 0-1

    // Error
    double err_x, err_y, err_a; // Anticipated max error in each x, y, a from last coordinate

    // Measurements
    struct sensor_vector *measurements;

    // Next chronological node
    struct graph_node *next;

} graph_node;

// Draw a circle on the screen
// Source: https://stackoverflow.com/questions/38334081/how-to-draw-circles-arcs-and-vector-graphics-in-sdl
void DrawCircle(SDL_Renderer *renderer, int32_t centreX, int32_t centreY, int32_t radius)
{
    const int32_t diameter = (radius * 2);

    int32_t x = (radius - 1);
    int32_t y = 0;
    int32_t tx = 1;
    int32_t ty = 1;
    int32_t error = (tx - diameter);

    while (x >= y)
    {
        //  Each of the following renders an octant of the circle
        SDL_RenderDrawPoint(renderer, centreX + x, centreY - y);
        SDL_RenderDrawPoint(renderer, centreX + x, centreY + y);
        SDL_RenderDrawPoint(renderer, centreX - x, centreY - y);
        SDL_RenderDrawPoint(renderer, centreX - x, centreY + y);
        SDL_RenderDrawPoint(renderer, centreX + y, centreY - x);
        SDL_RenderDrawPoint(renderer, centreX + y, centreY + x);
        SDL_RenderDrawPoint(renderer, centreX - y, centreY - x);
        SDL_RenderDrawPoint(renderer, centreX - y, centreY + x);

        if (error <= 0)
        {
            ++y;
            error += ty;
            ty += 2;
        }

        if (error > 0)
        {
            --x;
            tx += 2;
            error += (tx - diameter);
        }
    }
}

// Draw a ray to the screen given x,y,l,a
void DrawRay(SDL_Renderer *renderer, int32_t centreX, int32_t centreY, int32_t length, double angle)
{
    double dx = cosf(angle);
    double dy = sinf(angle);
    int out_x = dx * (double)length;
    int out_y = dy * (double)length;

    SDL_RenderDrawLine(renderer, centreX, centreY, centreX + out_x, centreY + out_y);
}

// Draw a point to the screen given x,y,l,a
void DrawPoint(SDL_Renderer *renderer, int32_t centreX, int32_t centreY, int32_t length, double angle)
{
    double dx = cosf(angle);
    double dy = sinf(angle);
    int out_x = dx * (double)length;
    int out_y = dy * (double)length;

    DrawCircle(renderer, centreX + out_x, centreY + out_y, 3);
}

// Returns a random number in the given range
double rand_in_range(double bottom, double top)
{
    double ret_val = ((double)rand() / (double)RAND_MAX) * (top - bottom) + bottom;
    if (ret_val > top || ret_val < bottom)
    {
        printf("%f t:%f b:%f\n", ret_val, top, bottom);
    }
    return ret_val;
}

// Compute the correct ray length given a set of collision objects (walls)
double get_ray_len(SDL_Rect (*walls)[], double x, double y, double angle)
{
    double dx = cosf(angle);
    double dy = sinf(angle);
    // Using int max here because double max overflows later when cast to int
    double length = INT_MAX;
    int i = 0;

    // Check every wall in the array
    while ((*walls)[i].x != -1)
    {
        double x_1 = (*walls)[i].x;
        double y_1 = (*walls)[i].y;
        double x_2 = (*walls)[i].w;
        double y_2 = (*walls)[i].h;

        // Calculates the determinant. If det = 0, the two lines are parallel
        // and will never collide.
        double det = dx * (y_2 - y_1) - dy * (x_2 - x_1);
        if (det != 0.0)
        {
            // Compute the length of the ray when it collides with the wall
            // (could be negative for rear-ward collisions)
            double ray_len = ((x_1 - x) * (y_2 - y_1) - (y_1 - y) * (x_2 - x_1)) / det;
            // Compute the length of the wall where the collision takes place.
            // Is between zero and 1 if the collision happens where the wall
            // exists.
            double wall_len = (dy * (x_1 - x) - dx * (y_1 - y)) / det;

            // Check that the collision is valid
            if (wall_len >= 0 && wall_len <= 1 && ray_len >= 0.0 && ray_len < length && ray_len >= 0)
            {
                length = ray_len;
            }
        }
        i++;
    }
    return length + rand_in_range(-length / 50, length / 50);
}

double safe_angle(double angle)
{
    if (angle < 0.0)
    {
        return angle + (abs(angle) / (2 * M_PI)) * 2 * M_PI;
    }
    else if (angle > 2 * M_PI)
    {
        return angle - (abs(angle) / (2 * M_PI)) * 2 * M_PI;
    }
    else
    {
        return angle;
    }
}

void drawRotatedRect(SDL_Renderer *renderer, int x, int y, int w, int h, double angle)
{
    double cosAngle = cosf(angle);
    double sinAngle = sinf(angle);

    SDL_Point points[5] = {
        {x - w, y + h},
        {x + w, y + h},
        {x + w, y - h},
        {x - w, y - h},
        {x - w, y + h}};

    for (int i = 0; i < 5; i++)
    {
        double dx = points[i].x - x;
        double dy = points[i].y - y;
        points[i].x = x + dx * cosAngle - dy * sinAngle;
        points[i].y = y + dx * sinAngle + dy * cosAngle;
    }

    SDL_RenderDrawLines(renderer, points, 5);
}

// Scale the input value between given output values, clamping if over/under
// max input.
double clamp_and_scale(double in_bottom, double in_top, double out_top, double input)
{
    double out_bottom = 0;
    double output;
    if (input >= in_top)
    {
        output = out_top;
    }
    else if (input <= out_bottom)
    {
        output = out_bottom;
    }
    else
    {
        input -= in_bottom;
        input /= in_top - in_bottom;
        input *= out_top - out_bottom;
        input += out_bottom;
        output = input;
    }
    return output;
}

double angle_between_points(double x1, double y1, double x2, double y2)
{
    double dx = x2 - x1;
    double dy = y2 - y1;
    return atan2(dy, dx) + M_PI;
}

double func_anime_gausian_bannana(double x, double y, double robot_x, double robot_y, double radius, double angle)
{
    double new_angle = (M_PI * 2) - angle + M_PI / 2;
    double x_new = x - robot_x;
    double y_new = y - robot_y;

    double x_rot = x_new * cosf(new_angle) - y_new * sinf(new_angle);
    double y_rot = x_new * sinf(new_angle) + y_new * cosf(new_angle);

    y_rot += -radius + sqrt(fmax(pow(radius, 2) - pow(x_rot, 2), 0.0));
    return 1 / (1 + pow(sqrt(pow(x_rot / 2, 2) + pow(y_rot, 2)) / (radius / 15), 2));
}

int main(int argc, char *argv[])
{
    struct agent robot;
    double speed_linear;
    double speed_angular;
    SDL_Event event;
    struct timespec time_start, time_end;
    int close;
    int function_draw;

    // Seed rand with current time for more random values
    srand(time(NULL));

    // Set the walls
    SDL_Rect walls[] = {
        // Border walls
        {10, 10, WINDOW_WIDTH - 10, 10},                                 // Top
        {10, WINDOW_HEIGHT - 10, WINDOW_WIDTH - 10, WINDOW_HEIGHT - 10}, // Bottom
        {10, 10, 10, WINDOW_HEIGHT - 10},                                // Left
        {WINDOW_WIDTH - 10, 10, WINDOW_WIDTH - 10, WINDOW_HEIGHT - 10},  // Right

        // Internal walls
        {400, 400, 600, 400},
        {400, 400, 400, 600},
        {-1, -1, -1, -1} // Sentinel value to make processing more efficient
    };

    // Init SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        printf("error initializing SDL: %s\n", SDL_GetError());
    }

    SDL_Window *window = SDL_CreateWindow("Real World", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);

    // Set flags
    Uint32 render_flags = SDL_RENDERER_ACCELERATED;

    // Create renderers
    SDL_Renderer *rend = SDL_CreateRenderer(window, -1, render_flags);

    // Allows us to read key state
    const Uint8 *keys = SDL_GetKeyboardState(NULL);

    // Set the speed variables
    speed_linear = 3;
    speed_angular = 0.03;

    // Set the robots initial position as random.
    // robot.x = rand_in_range(10, 990);
    // robot.y = rand_in_range(10, 990);
    // robot.angle = rand_in_range(0, 2 * M_PI);
    robot.x = 50;
    robot.y = 50;
    robot.angle = 0;
    robot.measurements = NULL;

    // Initialize the graph
    graph_node *graph_start = malloc(sizeof(graph_node));
    graph_start->x = 0;
    graph_start->y = 0;
    graph_start->err_x = 0;
    graph_start->err_y = 0;
    graph_start->err_a = 0;
    graph_start->angular = 0;
    graph_start->next = NULL;
    graph_node *graph_end = graph_start;

    // Set the paths inital coordinates
    connection path = {0, 0, 0, 500, 500, NULL};

    // Loop control
    close = 0;

    // Auto drive control
    function_draw = 0;

    // Index of the best particle
    int best = 0;

    // Start the clocks
    clock_gettime(CLOCK_MONOTONIC, &time_start);

    movement movement_estimate = {0, 0, 0, 0, 0};
    while (!close)
    {

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                // quit the loop
                close = 1;
                break;
            case SDL_MOUSEBUTTONDOWN:
                // Toggle function maping
                if (function_draw)
                {
                    function_draw--;
                }
                else
                {
                    function_draw++;
                }
                break;

                // Set path end to new coordinate
                path.x_2 = event.motion.x;
                path.y_2 = event.motion.y;
                break;
            }
        }

        // Process user input. This is done outside of the event handler to
        // correctly process holding down movement keys.
        double linear_noise = 0.1;
        double angular_noise = 0.2;
        double speed_linear_real = speed_linear + rand_in_range(-speed_linear*linear_noise,speed_linear*linear_noise);
        double speed_angular_real = speed_angular + rand_in_range(-speed_angular*angular_noise,speed_angular*angular_noise);
        if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])
        {
            robot.x += speed_linear_real * cosf(robot.angle);
            robot.y += speed_linear_real * sinf(robot.angle);

            // Update movement coordinates
            movement_estimate.x += speed_linear * cosf(movement_estimate.angular);
            movement_estimate.y += speed_linear * sin(movement_estimate.angular);
            movement_estimate.linear_total += speed_linear;
        }
        if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])
        {
            robot.x += -speed_linear_real * cosf(robot.angle);
            robot.y += -speed_linear_real * sinf(robot.angle);

            // Update movement coordinates
            movement_estimate.x += -speed_linear * cosf(movement_estimate.angular);
            movement_estimate.y += -speed_linear * sin(movement_estimate.angular);
            movement_estimate.linear_total += speed_linear;
        }
        if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])
        {
            robot.angle = fmod(robot.angle - speed_angular_real, 2 * M_PI);

            // Update angle coordinates
            movement_estimate.angular += -speed_angular;
            movement_estimate.angular_total += speed_angular;
        }
        if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT])
        {
            robot.angle = fmod(robot.angle + speed_angular_real, 2 * M_PI);

            // Update angle coordinates
            movement_estimate.angular += speed_angular;
            movement_estimate.angular_total += speed_angular;
        }

        // Calculate the lengths of the agent sensors
        struct sensor_vector *measurement_cur;
        measurement_cur = robot.measurements;
        if (measurement_cur != NULL)
        {
            while (1)
            {
                if (measurement_cur->next == NULL)
                {

                    free(measurement_cur);
                    break;
                }
                free(measurement_cur);
                struct sensor_vector *temp = measurement_cur->next;
                measurement_cur = temp;
            }
        }
        for (int i = 0; i < NUM_SENSORS; i++)
        {
            if (i == 0)
            {
                robot.measurements = malloc(sizeof(sensor_vector));
                measurement_cur = robot.measurements;
            }
            else
            {
                measurement_cur->next = malloc(sizeof(sensor_vector));
                measurement_cur = measurement_cur->next;
            }
            // measurement_cur->angle = -SENSOR_OFFSET + ((double)i / (NUM_SENSORS - 1)) * 2 * SENSOR_OFFSET;
            measurement_cur->angle = rand_in_range(-SENSOR_OFFSET, SENSOR_OFFSET);

            measurement_cur->length = get_ray_len(&walls, robot.x, robot.y, robot.angle + measurement_cur->angle);
        }
        measurement_cur->next = NULL;

        // Add new node to graph
        clock_gettime(CLOCK_MONOTONIC, &time_end);
        double elapsed = (time_end.tv_sec - time_start.tv_sec) + (time_end.tv_nsec - time_start.tv_nsec) / 1e9;
        if (elapsed >= (double)1 / (double)MAX_NODE_FREQUENCY)
        {
            double prev_err_x = graph_end->err_x;
            double prev_err_y = graph_end->err_y;
            double prev_err_a = graph_end->err_a;
            graph_end->next = malloc(sizeof(graph_node));
            graph_end = graph_end->next;
            // printf("x:%f y:%f\n",movement_estimate.x,movement_estimate.y);
            graph_end->x = movement_estimate.x;
            graph_end->y = movement_estimate.y;
            graph_end->err_x = prev_err_x * (1 + 0.0002 * movement_estimate.linear_total) + 0.01 * movement_estimate.linear_total;
            graph_end->err_y = prev_err_y * (1 + 0.0002 * movement_estimate.linear_total) + 0.01 * movement_estimate.linear_total;
            graph_end->err_a = (prev_err_a * 1.05 + 0.005) * (movement_estimate.linear_total / 100) + (prev_err_a * 1.05 + 0.005) * movement_estimate.angular_total;
            graph_end->angular = movement_estimate.angular;
            graph_end->next = NULL;

            // Sensors
            struct sensor_vector *measurement_out;
            if (robot.measurements != NULL)
            {

                measurement_cur = robot.measurements;
                int first = 1;
                while (measurement_cur != NULL)
                {
                    // printf("H\n");
                    if (first)
                    {
                        graph_end->measurements = malloc(sizeof(sensor_vector));
                        measurement_out = graph_end->measurements;
                        first = 0;
                    }
                    else
                    {
                        measurement_out->next = malloc(sizeof(sensor_vector));
                        measurement_out = measurement_out->next;
                    }
                    measurement_out->angle = measurement_cur->angle;
                    measurement_out->length = measurement_cur->length;
                    measurement_cur = measurement_cur->next;
                }
                measurement_out->next = NULL;
            }
            else
            {
                graph_end->measurements = NULL;
            }

            // printf("D\n");

            // Clear the movement estimate
            movement_estimate.angular_total = 0;
            movement_estimate.linear_total = 0;
            movement_estimate.angular = 0;
            movement_estimate.x = 0;
            movement_estimate.y = 0;
            clock_gettime(CLOCK_MONOTONIC, &time_start);
        }

        // Stop clock (we don't want to count draw times)
        // t = clock() - t;

        // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
        //                       BEGIN GRAPHICS
        // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

        // Clear the screen
        SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
        SDL_RenderClear(rend);

        // Draw the graph
        double x = 50;
        double y = 50;
        double x_prev = x;
        double y_prev = y;
        double angle = 0;
        graph_node *graph_cur = graph_start;
        while (graph_cur != NULL)
        {
            SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
            x += graph_cur->x * cosf(angle) - graph_cur->y * sinf(angle);
            y += graph_cur->x * sinf(angle) + graph_cur->y * cosf(angle);
            angle += graph_cur->angular;
            // printf("x:%a y:%a a:%a, t:%a, T:%a\n", graph_cur->y, sinf(angle), angle, graph_cur->y * sinf(angle), graph_cur->y * sinf(safe_angle(angle)));
            // DrawCircle(rend, x, y, 5);
            if (!function_draw)
            {
                SDL_RenderDrawLine(rend, x, y, x_prev, y_prev);

                // Draw the measurements
                SDL_SetRenderDrawColor(rend, 0, 100, 255, 255);
                measurement_cur = graph_cur->measurements;
                while (measurement_cur != NULL)
                {
                    // printf("1\n");
                    // printf("l:%f a:%f\n",measurement_cur->length,measurement_cur->angle);
                    DrawPoint(rend, x, y, measurement_cur->length, angle + measurement_cur->angle);
                    measurement_cur = measurement_cur->next;
                }
            }

            // Draw error estimate
            // SDL_SetRenderDrawColor(rend, 0, 100, 0, 255);
            // drawRotatedRect(rend, x, y, graph_cur->err_x, graph_cur->err_y, angle);
            // printf("x:%f y:%f a:%f\n", graph_cur->err_x, graph_cur->err_y, graph_cur->err_a);

            graph_cur = graph_cur->next;
            x_prev = x;
            y_prev = y;
        }

        // Draw the function
        if (function_draw)
        {
            double angle = angle_between_points(50, 50, x, y);
            // printf("A:%f\n", angle);
            for (int i = 0; i < WINDOW_WIDTH; i++)
            {
                for (int j = 0; j < WINDOW_HEIGHT; j++)
                {
                    SDL_SetRenderDrawColor(rend, 0, clamp_and_scale(0, 1, 255, func_anime_gausian_bannana(i, j, x, y, 100, angle)), 0, 255);
                    SDL_RenderDrawPoint(rend, i, j);
                }
            }
        }

        // Draw walls
        SDL_SetRenderDrawColor(rend, 0, 50, 50, 255);
        for (int i = 0; i < sizeof(walls) / sizeof(SDL_Rect); i++)
        {
            SDL_RenderDrawLine(rend, walls[i].x, walls[i].y, walls[i].w, walls[i].h);
        }

        // Draw robot
        SDL_SetRenderDrawColor(rend, 255, 255, 255, 255);
        DrawCircle(rend, robot.x, robot.y, 15);
        SDL_SetRenderDrawColor(rend, 100, 0, 0, 255);
        if (robot.measurements != NULL)
        {

            measurement_cur = robot.measurements;
            while (1)
            {
                DrawRay(rend, robot.x, robot.y, measurement_cur->length, robot.angle + measurement_cur->angle);
                if (measurement_cur->next == NULL)
                {
                    break;
                }
                measurement_cur = measurement_cur->next;
            }
        }

        // Render
        SDL_RenderPresent(rend);

        // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
        //                       END GRAPHICS
        // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

        // printf("\rprocessing (not including graphics) took %f seconds to execute", ((double)t) / CLOCKS_PER_SEC);
        // fflush(stdout);

        // Start new clock
        // t = clock();
    }
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
