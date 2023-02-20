#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <math.h>
#include <limits.h>
#include <time.h>

// export LD_LIBRARY_PATH="/usr/local/lib"
// gcc main.c -o main.o `sdl2-config --cflags --libs` -lSDL2_image -lm

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 1000
#define NUM_PARTICLES 5000
#define SENSOR_OFFSET 0.2

typedef struct agent
{
    double x;
    double y;
    double angle;    // Rotation in radians
    double length_c; // length of the sensor ray
    double length_l; // length of the sensor ray
    double length_r; // length of the sensor ray
} agent;

typedef struct particle
{
    double x;
    double y;
    double angle;
    double weight;
} particle;

typedef struct movement
{
    double x;
    double y;
    double linear;
    double angular;
} movement;

typedef struct connection
{
    double h;
    double x_1;
    double y_1;
    double x_2;
    double y_2;
    struct connection *next;
} connection;

typedef struct double_suggestion
{
    int intersection;
    double x_1;
    double y_1;
    double x_2;
    double y_2;
} double_suggestion;

// https://stackoverflow.com/questions/38334081/how-to-draw-circles-arcs-and-vector-graphics-in-sdl
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

void DrawRay(SDL_Renderer *renderer, int32_t centreX, int32_t centreY, int32_t length, double angle)
{
    double dx = cosf(angle);
    double dy = sinf(angle);
    // printf("angle %f\n",angle);
    // printf("dx,dy %f,%f\n",dx,dy);
    int out_x = dx * (double)length;
    int out_y = dy * (double)length;
    // printf("out_x,out_y %d,%d\n",out_x,out_y);

    SDL_RenderDrawLine(renderer, centreX, centreY, centreX + out_x, centreY + out_y);
}

double get_ray_len(SDL_Rect (*walls)[], double x, double y, double angle)
{
    double dx = cosf(angle);
    double dy = sinf(angle);
    double length = INT_MAX; // Using int max here because double max overflows later when cast to int
    int i = 0;
    while ((*walls)[i].x != -1)
    {
        // printf("i:%d\n",i);
        double x_1 = (*walls)[i].x;
        double y_1 = (*walls)[i].y;
        double x_2 = (*walls)[i].w;
        double y_2 = (*walls)[i].h;

        double det = dx * (y_2 - y_1) - dy * (x_2 - x_1);

        // printf("det %f\n",det);
        if (det != 0.0)
        {
            double ray_len = ((x_1 - x) * (y_2 - y_1) - (y_1 - y) * (x_2 - x_1)) / det;
            double wall_len = (dy * (x_1 - x) - dx * (y_1 - y)) / det;

            // printf("wall_len, ray_len %f,%f\n",wall_len, ray_len);
            if (wall_len >= 0 && wall_len <= 1 && ray_len >= 0.0 && ray_len < length && ray_len >= 0)
            {
                length = ray_len;
            }
        }
        i++;
    }
    // printf("len: %f\n",length);
    return length;
}

// returns NULL if there is no intersection, otherwise returns a struct of points that avoid the collision
double_suggestion path_intersects(SDL_Rect (*walls)[], struct connection *path)
{
    // soh cah toa
    double input_length = sqrt(pow(path->x_2 - path->x_1, 2) + pow(path->y_2 - path->y_1, 2));
    double dx = (path->x_2 - path->x_1) / input_length;
    double dy = (path->y_2 - path->y_1) / input_length;
    double min_length = INT_MAX;
    double_suggestion output;
    output.intersection = 0;
    // printf("l:%F dx:%f dy:%f\n",length, dx,dy);
    double x = path->x_1;
    double y = path->y_1;

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

                // printf("l1:%F l2:%f\n",input_length, ray_len);
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

double rand_in_range(double bottom, double top)
{
    double ret_val = ((double)rand() / (double)RAND_MAX) * (top - bottom) + bottom;
    if (ret_val > top || ret_val < bottom)
    {
        printf("%f t:%f b:%f\n", ret_val, top, bottom);
    }
    return ret_val;
}

void predict_particles(particle (*particles)[NUM_PARTICLES], movement movement_estimate)
{
    // printf("x:%f y:%f a:%f\n", (*particles)[1].x, (*particles)[1].y, (*particles)[1].angle);

    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        (*particles)[i].x += movement_estimate.linear * cosf((*particles)[i].angle);
        (*particles)[i].y += movement_estimate.linear * sinf((*particles)[i].angle);
        (*particles)[i].angle = fmod((*particles)[i].angle + movement_estimate.angular, 2 * M_PI);
        if ((*particles)[i].angle < 0)
        {
            (*particles)[i].angle += 2 * M_PI;
        }
    }

    // for (int i = 0; i < NUM_PARTICLES; i++)
    // {
    //     (*particles)[i].x += movement_estimate.x;
    //     (*particles)[i].y += movement_estimate.y;
    // }
}

double get_normal(double half_life, double target, double input)
{
    return 1 / (1 + pow((input - target) / half_life, 2));
}

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
            (*particles)[i].weight = get_normal(10, robot.length_c, get_ray_len(walls, (*particles)[i].x, (*particles)[i].y, (*particles)[i].angle));
            (*particles)[i].weight += get_normal(10, robot.length_r, get_ray_len(walls, (*particles)[i].x, (*particles)[i].y, (*particles)[i].angle + SENSOR_OFFSET));
            (*particles)[i].weight += get_normal(10, robot.length_l, get_ray_len(walls, (*particles)[i].x, (*particles)[i].y, (*particles)[i].angle - SENSOR_OFFSET));

            // Simple (cheating)
            // (*particles)[i].weight = get_normal(50, robot.x, (*particles)[i].x);
            // (*particles)[i].weight += get_normal(50, robot.y, (*particles)[i].y);
            weight_sum += (*particles)[i].weight;
        }
    }
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

void resample_particles(particle (*particles)[NUM_PARTICLES])
{
    double rate_angular = 0.05;
    double rate_linear = 2;
    particle resampled_particles[NUM_PARTICLES];
    double random_offset = (double)rand() / (double)RAND_MAX;
    double avg_particle_weight = 1.0 / (double)NUM_PARTICLES;
    int j = 0;
    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        // printf("weight:%f\n", (*particles)[j].weight);
        if ((double)rand() / (double)RAND_MAX < 0.99)
        {
            double cumulative_weight = 0.0;
            double u = random_offset + (double)i * avg_particle_weight;
            while (u > cumulative_weight)
            {
                // printf("weight:%f\n", (*particles)[j].weight);
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
            // printf("%f\n",(2*((double)rand()/(double)RAND_MAX) -1));
            resampled_particles[i].x = (*particles)[j].x + rate_linear * (2 * ((double)rand() / (double)RAND_MAX) - 1);
            resampled_particles[i].y = (*particles)[j].y + rate_linear * (2 * ((double)rand() / (double)RAND_MAX) - 1);
            resampled_particles[i].angle = (*particles)[j].angle + rate_angular * (2 * ((double)rand() / (double)RAND_MAX) - 1);
            // resampled_particles[i].angle = (*particles)[j].angle;
        }
        else
        {
            resampled_particles[i].x = rand_in_range(10, 990);
            resampled_particles[i].y = rand_in_range(10, 990);
            resampled_particles[i].angle = rand_in_range(0, 2 * M_PI);
        }
    }
    memcpy(particles, resampled_particles, sizeof(resampled_particles));
}

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
        // printf("NO INTERSECTION\n");
        path_refactor(walls, path->next);
    }
    else
    {
        // printf("INTERSECTION\n");
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
    // printf("%f,%f\n",path->x_2,path->y_2);
}

int main(int argc, char *argv[])
{
    SDL_Rect walls[] = {
        {10, 10, WINDOW_WIDTH - 10, 10},                                 // Top
        {10, WINDOW_HEIGHT - 10, WINDOW_WIDTH - 10, WINDOW_HEIGHT - 10}, // Bottom
        {10, 10, 10, WINDOW_HEIGHT - 10},                                // Left
        {WINDOW_WIDTH - 10, 10, WINDOW_WIDTH - 10, WINDOW_HEIGHT - 10},  // Right

        {400, 400, 600, 400},
        {400, 400, 400, 600},
        {-1, -1, -1, -1} // Sentinel value to make processing more efficient
    };

    particle particles[NUM_PARTICLES];

    // Init the particles
    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        particles[i].x = rand_in_range(10, 990);
        particles[i].y = rand_in_range(10, 990);
        particles[i].angle = rand_in_range(0, 2 * M_PI);
        particles[i].weight = (double)1 / (double)NUM_PARTICLES;
        // particles[i].weight = 1;
    }

    // returns zero on success else non-zero
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        printf("error initializing SDL: %s\n", SDL_GetError());
    }
    SDL_Window *win = SDL_CreateWindow("Monte-Carlo Localization", // creates a window
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                       WINDOW_WIDTH, WINDOW_HEIGHT, 0);

    // triggers the program that controls
    // your graphics hardware and sets flags
    Uint32 render_flags = SDL_RENDERER_ACCELERATED;

    // creates a renderer to render our images
    SDL_Renderer *rend = SDL_CreateRenderer(win, -1, render_flags);

    // let us control our image position
    // so that we can move it with our keyboard.
    struct agent robot;
    robot.x = 100;
    robot.y = 100;
    robot.angle = 0;
    double speed_linear = 5;
    double speed_angular = 0.05;
    double max_weight = 0;

    int close = 0;
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    SDL_Event event;
    int resample = 1;
    clock_t t;

    t = clock();
    connection path = {0, 0, 0, 500, 500, NULL};
    while (!close)
    {
        // Move the end of the path to the first node so we can recalculate the movement
        struct connection *current_connection = &path;
        while (current_connection->next !=NULL)
        {
            path.x_2 = current_connection->next->x_2;
            path.y_2 = current_connection->next->y_2;
            current_connection = current_connection->next;
        }
        path.next = NULL;

        movement movement_estimate = {0, 0, 0, 0};
        // Process events
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                // handling of close button
                close = 1;
                break;
            case SDL_MOUSEBUTTONDOWN:
                // if (resample)
                // {
                //     resample--;
                // }
                // else
                // {
                //     resample++;
                // }
                // break;

                // SDL_GetMouseState(x, y);
                // printf("X,y %d,%d\n",event.motion.x,event.motion.y);
                path.x_2 = event.motion.x;
                path.y_2 = event.motion.y;
                break;
            }
        }

        // Process user input
        if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])
        {
            robot.x += speed_linear * cosf(robot.angle);
            robot.y += speed_linear * sinf(robot.angle);

            movement_estimate.x = speed_linear * cosf(robot.angle);
            movement_estimate.y = speed_linear * sinf(robot.angle);

            movement_estimate.linear = speed_linear;
        }
        if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])
        {
            robot.x += -speed_linear * cosf(robot.angle);
            robot.y += -speed_linear * sinf(robot.angle);

            movement_estimate.x = -speed_linear * cosf(robot.angle);
            movement_estimate.y = -speed_linear * sinf(robot.angle);

            movement_estimate.linear = -speed_linear;
        }
        if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])
        {
            robot.angle = fmod(robot.angle - speed_angular, 2 * M_PI);

            movement_estimate.angular += -speed_angular;
        }
        if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT])
        {
            robot.angle = fmod(robot.angle + speed_angular, 2 * M_PI);

            movement_estimate.angular += speed_angular;
        }

        // Make calculations for localization
        robot.length_c = get_ray_len(&walls, robot.x, robot.y, robot.angle);
        robot.length_r = get_ray_len(&walls, robot.x, robot.y, robot.angle + SENSOR_OFFSET);
        robot.length_l = get_ray_len(&walls, robot.x, robot.y, robot.angle - SENSOR_OFFSET);
        predict_particles(&particles, movement_estimate);
        max_weight = calc_weights(&particles, &walls, robot);

        // find the best-guess robot
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

        // Process the screen
        SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
        SDL_RenderClear(rend);

        t = clock() - t;

        // Draw walls
        SDL_SetRenderDrawColor(rend, 0, 255, 255, 255);
        for (int i = 0; i < sizeof(walls) / sizeof(SDL_Rect); i++)
        {
            SDL_RenderDrawLine(rend, walls[i].x, walls[i].y, walls[i].w, walls[i].h);
        }

        // Draw particles
        // printf("x,y %d,%d,%d\n",(int)particles[1].x, (int)particles[1].y ,(int)particles[1].angle);
        for (int i = 0; i < NUM_PARTICLES; i++)
        {
            // SDL_SetRenderDrawColor(rend, ((double)1 - (particles[i].weight / max_weight)) * 255, (particles[i].weight / max_weight) * 255, 0, 255);
            SDL_SetRenderDrawColor(rend, 0, (particles[i].weight / max_weight) * 255, 0, 255);
            DrawCircle(rend, particles[i].x, particles[i].y, 2);
        }

        // Draw path
        // current_connection = &path;
        // SDL_SetRenderDrawColor(rend, 255, 255, 255, 255);
        // do
        // {
        //     SDL_RenderDrawLine(rend, current_connection->x_1, current_connection->y_1, current_connection->x_2, current_connection->y_2);
        //     current_connection = current_connection->next;
        // } while (current_connection != NULL);

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

        printf("\rprocessing (not including drawing to screen) took %f seconds to execute", ((double)t) / CLOCKS_PER_SEC);
        fflush(stdout);

        t = clock();
        // More calculations
        if (resample)
        {
            resample_particles(&particles);
        }
        // calculates to 60 fps
        // SDL_Delay(1000 / 60);
    }
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}
