// Nico Zucca, 1/2023

// Compile cmd:
// export LD_LIBRARY_PATH="/usr/local/lib"
// gcc main.c -o main.o `sdl2-config --cflags --libs` -lSDL2_image -lm -O3

#include <SDL2/SDL_image.h>
#include <math.h>
#include <limits.h>
#include <time.h>

// Height of the window
#define WINDOW_WIDTH 1000
// Width of the window
#define WINDOW_HEIGHT 1000
// Number of particles to compute. Default 5000
#define NUM_PARTICLES 5000
// Sensor offset for the side sensors in radians
#define SENSOR_OFFSET 0.2

// Agent object.
typedef struct agent
{
    double x;
    double y;
    double angle;    // Rotation in radians
    double length_c; // length of the sensor ray
    double length_l; // length of the sensor ray
    double length_r; // length of the sensor ray
} agent;

// Particle object
typedef struct particle
{
    double x;
    double y;
    double angle;
    double weight;
} particle;

// Used to store anticipated movement
typedef struct movement
{
    double x;
    double y;
    double linear;
    double angular;
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

// Return structure used by path finding algorithm
typedef struct double_suggestion
{
    int intersection;
    double x_1;
    double y_1;
    double x_2;
    double y_2;
} double_suggestion;

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
    return length;
}

// Check if the path has any intersections with given walls
double_suggestion path_intersects(SDL_Rect (*walls)[], struct connection *path)
{
    // soh cah toa :)
    double input_length;
    double dx;
    double dy;
    double x;
    double y;
    double min_length = INT_MAX;
    double_suggestion output;
    output.intersection = 0;
    input_length = sqrt(pow(path->x_2 - path->x_1, 2) + pow(path->y_2 - path->y_1, 2));
    dx = (path->x_2 - path->x_1) / input_length;
    dy = (path->y_2 - path->y_1) / input_length;

    x = path->x_1;
    y = path->y_1;

    int i = 0;
    while ((*walls)[i].x != -1)
    {
        double x_1 = (*walls)[i].x;
        double y_1 = (*walls)[i].y;
        double x_2 = (*walls)[i].w;
        double y_2 = (*walls)[i].h;

        double det = dx * (y_2 - y_1) - dy * (x_2 - x_1);

        if (det != 0.0)
        {
            double ray_len = ((x_1 - x) * (y_2 - y_1) - (y_1 - y) * (x_2 - x_1)) / det;
            double wall_len = (dy * (x_1 - x) - dx * (y_1 - y)) / det;

            if (wall_len >= 0 && wall_len <= 1 && ray_len >= 0.0 && ray_len < input_length && ray_len < min_length && ray_len >= 0)
            {
                // Return coordinates of the side of each wall, to allow path to find way around
                min_length = ray_len;
                output.intersection = 1;
                output.x_1 = x_1;
                output.x_2 = x_2;
                output.y_1 = y_1;
                output.y_2 = y_2;
            }
        }
        i++;
    }
    return output;
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

// Predict the movement of the particles given a movement criteria
void predict_particles(particle (*particles)[NUM_PARTICLES], movement movement_estimate)
{
    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        (*particles)[i].x += movement_estimate.linear * cosf((*particles)[i].angle);
        (*particles)[i].y += movement_estimate.linear * sinf((*particles)[i].angle);
        (*particles)[i].angle = fmod((*particles)[i].angle + movement_estimate.angular, 2 * M_PI);

        // Keep angle in range
        if ((*particles)[i].angle < 0)
        {
            (*particles)[i].angle += 2 * M_PI;
        }
    }
}

// Return a sample from a normal distribution given a half-life (std-dev) and
// target.
double get_normal(double half_life, double target, double input)
{
    return 1 / (1 + pow((input - target) / half_life, 2));
}

// Calculate the weights of the particles based on the sensor readings of the
// agent and each pixel.
double calc_weights(particle (*particles)[NUM_PARTICLES], SDL_Rect (*walls)[], agent robot)
{
    double weight_sum = 0;
    double max_weight = 0;
    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        if ((*particles)[i].x < 0 || (*particles)[i].y < 0 || (*particles)[i].x > 1000 || (*particles)[i].y > 1000)
        {
            (*particles)[i].weight = 0;
        }
        else
        {
            // Complicated, only uses sensor inputs.
            (*particles)[i].weight = get_normal(10, robot.length_c, get_ray_len(walls, (*particles)[i].x, (*particles)[i].y, (*particles)[i].angle));
            (*particles)[i].weight += get_normal(10, robot.length_r, get_ray_len(walls, (*particles)[i].x, (*particles)[i].y, (*particles)[i].angle + SENSOR_OFFSET));
            (*particles)[i].weight += get_normal(10, robot.length_l, get_ray_len(walls, (*particles)[i].x, (*particles)[i].y, (*particles)[i].angle - SENSOR_OFFSET));

            // Simple (cheating) for debugging
            // (*particles)[i].weight = get_normal(50, robot.x, (*particles)[i].x);
            // (*particles)[i].weight += get_normal(50, robot.y, (*particles)[i].y);
            weight_sum += (*particles)[i].weight;
        }
    }

    // Make sure the weights sum to 1, and find the largest weight to return
    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        (*particles)[i].weight /= weight_sum;
        if ((*particles)[i].weight > max_weight)
        {
            max_weight = (*particles)[i].weight;
        }
    }
    return max_weight;
}

// Resample the particles based on their weights, with some randomness.
void resample_particles(particle (*particles)[NUM_PARTICLES])
{
    double rate_angular;
    double rate_linear;
    double random_offset;
    double avg_particle_weight;
    int j;
    particle resampled_particles[NUM_PARTICLES];

    // Random offset
    random_offset = (double)rand() / (double)RAND_MAX;

    // Randomness spread values for linear and angular values
    rate_angular = 0.05; // Radians
    rate_linear = 2;

    // The average particle weight (note that they all sum to 1)
    avg_particle_weight = 1.0 / (double)NUM_PARTICLES;

    j = 0;
    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        // "Scatter" value. Some percentage of values should be completely
        // random to allow for incorrect assumptions to be corrected.
        if ((double)rand() / (double)RAND_MAX < 0.99)
        {
            // This section is rather complicated and hard to explain, but it
            // randomly selects values proportionally to their weight, while
            // also guaranteeing an even spread of values between various
            // indexes (No cluster degeneracy).
            double cumulative_weight = 0.0;
            double u = random_offset + (double)i * avg_particle_weight;
            while (u > cumulative_weight)
            {
                cumulative_weight += (*particles)[j].weight;
                j++;
                if (j >= NUM_PARTICLES)
                {
                    j = 0;
                }
            }
            if (j == 0)
            {
                j = NUM_PARTICLES - 1;
            }
            else
            {
                j--;
            }
            resampled_particles[i].x = (*particles)[j].x + rate_linear * (2 * ((double)rand() / (double)RAND_MAX) - 1);
            resampled_particles[i].y = (*particles)[j].y + rate_linear * (2 * ((double)rand() / (double)RAND_MAX) - 1);
            resampled_particles[i].angle = (*particles)[j].angle + rate_angular * (2 * ((double)rand() / (double)RAND_MAX) - 1);
        }
        else
        {
            // Random in range
            resampled_particles[i].x = rand_in_range(10, 990);
            resampled_particles[i].y = rand_in_range(10, 990);
            resampled_particles[i].angle = rand_in_range(0, 2 * M_PI);
        }
    }
    // Replace all particles with new resampled ones
    memcpy(particles, resampled_particles, sizeof(resampled_particles));
}

// Path planning using only linear conditions
void path_refactor(SDL_Rect (*walls)[], struct connection *path)
{
    // Base case
    if (path == NULL)
    {
        return;
    }

    // Recursive case
    struct double_suggestion new_points = path_intersects(walls, path);
    if (!new_points.intersection)
    {
        // Non-intersection case
        path_refactor(walls, path->next);
    }
    else
    {
        // Intersection case
        struct connection *new_connection = malloc(sizeof(connection));
        new_connection->next = path->next;
        path->next = new_connection;
        new_connection->x_1 = new_points.x_1;
        new_connection->y_1 = new_points.y_1;
        new_connection->x_2 = path->x_2;
        new_connection->y_2 = path->y_2;
        path->x_2 = new_connection->x_1;
        path->y_2 = new_connection->y_1;
    }
}

int main(int argc, char *argv[])
{
    struct agent robot;
    clock_t t;
    double speed_linear;
    double speed_angular;
    double max_weight;
    SDL_Event event;
    int close;
    int resample;
    particle particles[NUM_PARTICLES];

    // Seed rand with current time for more random values
    srand ( time(NULL) );

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

    // Init the particles
    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        particles[i].x = rand_in_range(10, 990);
        particles[i].y = rand_in_range(10, 990);
        particles[i].angle = rand_in_range(0, 2 * M_PI);
        particles[i].weight = (double)1 / (double)NUM_PARTICLES;
    }

    // Init SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        printf("error initializing SDL: %s\n", SDL_GetError());
    }
    SDL_Window *win = SDL_CreateWindow("Monte-Carlo Localization", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);

    // Set flags
    Uint32 render_flags = SDL_RENDERER_ACCELERATED;

    // Create a renderer
    SDL_Renderer *rend = SDL_CreateRenderer(win, -1, render_flags);

    // Allows us to read key state
    const Uint8 *keys = SDL_GetKeyboardState(NULL);

    // Set the speed variables
    speed_linear = 5;
    speed_angular = 0.05;

    // Set the robots initial position.
    robot.x = rand_in_range(10, 990);
    robot.y = rand_in_range(10, 990);
    robot.angle = rand_in_range(0, 2 * M_PI);
    
    // Set the paths inital coordinates
    connection path = {0, 0, 0, 500, 500, NULL};

    // Loop control
    close = 0;
    
    // Resample control
    resample = 1;
    
    // Used for drawing the particle colors
    max_weight = 0;

    // Start the clock
    t = clock();
    while (!close)
    {
        // Move the end of the path to the first node so we can recalculate the 
        // path every frame
        struct connection *current_connection = &path;
        while (current_connection->next != NULL)
        {
            path.x_2 = current_connection->next->x_2;
            path.y_2 = current_connection->next->y_2;
            current_connection = current_connection->next;
        }
        path.next = NULL;

        // Clear the movement estimate
        movement movement_estimate = {0, 0, 0, 0};

        // Process events
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                // quit the loop
                close = 1;
                break;
            case SDL_MOUSEBUTTONDOWN:
                // Toggle resampleing 
                // if (resample)
                // {
                //     resample--;
                // }
                // else
                // {
                //     resample++;
                // }
                // break;

                // Set path end to new coordinate
                path.x_2 = event.motion.x;
                path.y_2 = event.motion.y;
                break;
            }
        }

        // Process user input. This is done outside of the event handler to 
        // correctly process holding down movement keys.
        if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])
        {
            robot.x += speed_linear * cosf(robot.angle);
            robot.y += speed_linear * sinf(robot.angle);

            // Update cheater coordinates
            movement_estimate.x = speed_linear * cosf(robot.angle);
            movement_estimate.y = speed_linear * sinf(robot.angle);

            // Update actual movement coordinates
            movement_estimate.linear = speed_linear;
        }
        if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])
        {
            robot.x += -speed_linear * cosf(robot.angle);
            robot.y += -speed_linear * sinf(robot.angle);

            // Update cheater coordinates
            movement_estimate.x = -speed_linear * cosf(robot.angle);
            movement_estimate.y = -speed_linear * sinf(robot.angle);

            // Update actual movement coordinates
            movement_estimate.linear = -speed_linear;
        }
        if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])
        {
            robot.angle = fmod(robot.angle - speed_angular, 2 * M_PI);

            // Update actual angle coordinates
            movement_estimate.angular += -speed_angular;
        }
        if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT])
        {
            robot.angle = fmod(robot.angle + speed_angular, 2 * M_PI);

            // Update actual angle coordinates
            movement_estimate.angular += speed_angular;
        }

        // Calculate the lengths of the agent sensors
        robot.length_c = get_ray_len(&walls, robot.x, robot.y, robot.angle);
        robot.length_r = get_ray_len(&walls, robot.x, robot.y, robot.angle + SENSOR_OFFSET);
        robot.length_l = get_ray_len(&walls, robot.x, robot.y, robot.angle - SENSOR_OFFSET);

        // Move particles and update weights
        predict_particles(&particles, movement_estimate);
        max_weight = calc_weights(&particles, &walls, robot);

        // find the best-guess particle for drawing
        double weight = 0;
        int best = 0;
        for (int i = 0; i < NUM_PARTICLES; i++)
        {
            if (particles[i].weight > weight)
            {
                best = i;
                weight = particles[i].weight;
            }
        }

        // calculations for pathfinding
        path.x_1 = robot.x;
        path.y_1 = robot.y;
        path_refactor(&walls, &path);

        // Stop clock (we don't want to count draw times)
        t = clock() - t;

        // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
        //                       BEGIN GRAPHICS
        // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 

        // Process the screen
        SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
        SDL_RenderClear(rend);

        // Draw walls
        SDL_SetRenderDrawColor(rend, 0, 255, 255, 255);
        for (int i = 0; i < sizeof(walls) / sizeof(SDL_Rect); i++)
        {
            SDL_RenderDrawLine(rend, walls[i].x, walls[i].y, walls[i].w, walls[i].h);
        }

        // Draw particles
        for (int i = 0; i < NUM_PARTICLES; i++)
        {
            SDL_SetRenderDrawColor(rend, 0, (particles[i].weight / max_weight) * 255, 0, 255);
            DrawCircle(rend, particles[i].x, particles[i].y, 2);
        }

        // Draw path
        current_connection = &path;
        SDL_SetRenderDrawColor(rend, 255, 255, 255, 255);
        do
        {
            SDL_RenderDrawLine(rend, current_connection->x_1, current_connection->y_1, current_connection->x_2, current_connection->y_2);
            current_connection = current_connection->next;
        } while (current_connection != NULL);

        // Draw robot
        SDL_SetRenderDrawColor(rend, 255, 255, 255, 255);
        DrawCircle(rend, robot.x, robot.y, 15);
        SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
        DrawRay(rend, robot.x, robot.y, robot.length_c, robot.angle);
        DrawRay(rend, robot.x, robot.y, robot.length_r, robot.angle + SENSOR_OFFSET);
        DrawRay(rend, robot.x, robot.y, robot.length_l, robot.angle - SENSOR_OFFSET);

        // Draw best-guess robot
        SDL_SetRenderDrawColor(rend, 100, 255, 100, 255);
        DrawCircle(rend, particles[best].x, particles[best].y, 15);
        DrawRay(rend, particles[best].x, particles[best].y, get_ray_len(&walls, particles[best].x, particles[best].y, particles[best].angle), particles[best].angle);

        SDL_RenderPresent(rend);

        
        // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
        //                       END GRAPHICS
        // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 

        printf("\rprocessing (not including graphics) took %f seconds to execute", ((double)t) / CLOCKS_PER_SEC);
        fflush(stdout);

        // Start new clock
        t = clock();

        // Resample particles (this is done after graphics so that weight 
        // displays are correct for graphics)
        if (resample)
        {
            resample_particles(&particles);
        }
    }
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}
