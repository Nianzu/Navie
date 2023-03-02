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

typedef struct point
{
    double x;
    double y;
} point;

typedef struct cloud
{
    struct point *points;
    struct point com;
    int num;
} cloud;

typedef struct transform
{
    double x;
    double y;
    double a;
} transform;

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

void cloud_init(struct cloud *output, struct point *points, int num_points)
{
    output->num = num_points;
    output->com.x = 0;
    output->com.y = 0;
    output->points = malloc(sizeof(point) * num_points);
    for (int i = 0; i < num_points; i++)
    {
        output->com.x += points[i].x;
        output->com.y += points[i].y;
        output->points[i].x = points[i].x;
        output->points[i].y = points[i].y;
    }

    output->com.x /= num_points;
    output->com.y /= num_points;
}

double distance(double x1, double y1, double x2, double y2)
{
    double dx = x2 - x1;
    double dy = y2 - y1;
    return sqrt(dx * dx + dy * dy);
}

point closest_point(cloud *point_cloud, point p)
{
    double min_dist = DBL_MAX;
    int min_index = 0;
    for (int i = 0; i < point_cloud->num; i++)
    {
        double d = distance(point_cloud->points[i].x, point_cloud->points[i].y, p.x, p.y);
        if (d < min_dist)
        {
            min_dist = d;
            min_index = i;
        }
    }
    return point_cloud->points[min_index];
}

double calculate_error(cloud *a, cloud *c)
{
    double error = 0;
    for (int i = 0; i < c->num; i++)
    {
        point closest = closest_point(a, c->points[i]);
        // printf("(%f,%f) (%f,%f)\n",closest.x, closest.y, c->points[i].x, c->points[i].y);
        error += distance(closest.x, closest.y, c->points[i].x, c->points[i].y);
    }
    // printf("E:%f N:%d div:%f\n", error, c->num, error / c->num);
    return error / c->num;
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

double thermal_energy(double energy, double temperature)
{
    return pow(M_E, -energy / temperature);
}

point rotate_point(point in, point center, double angle)
{
    point out;
    double cosAngle = cos(angle);
    double sinAngle = sin(angle);
    double dx = in.x - center.x;
    double dy = in.y - center.y;

    out.x = center.x + dx * cosAngle - dy * sinAngle;
    out.y = center.y + dx * sinAngle + dy * cosAngle;

    return out;
}

point rotate_point_matrix(point in, point center, double (*rotation)[4])
{
    point out;
    double dx = in.x - center.x;
    double dy = in.y - center.y;

    out.x = center.x + dx * (*rotation)[0] + dy * (*rotation)[1];
    out.y = center.y + dx * (*rotation)[2] + dy * (*rotation)[3];

    return out;
}

void rotate_cloud(cloud *c, double angle)
{
    for (int i = 0; i < c->num; i++)
    {
        c->points[i] = rotate_point(c->points[i], c->com, angle);
    }
}

void rotate_cloud_matrix(cloud *c, double (*rotation)[4])
{
    for (int i = 0; i < c->num; i++)
    {
        c->points[i] = rotate_point_matrix(c->points[i], c->com, rotation);
    }
}

void apply_transform(cloud *c, transform t)
{
    for (int i = 0; i < c->num; i++)
    {
        c->points[i].x += t.x;
        c->points[i].y += t.y;
    }
    c->com.x += t.x;
    c->com.y += t.y;
    rotate_cloud(c, t.a);
}

double angle_between_points(point a, point b, point center)
{
    double ax = a.x - center.x;
    double ay = a.y - center.y;
    double bx = b.x - center.x;
    double by = b.y - center.y;

    double dot_product = ax * bx + ay * by;
    double cross_product = ax * by - ay * bx;

    return atan2(cross_product, dot_product) + M_PI;
}

// Get the covariance of c1 relative to c2
void compute_covariance(cloud *c1, cloud *c2, double (*covariance)[4])
{

    for (int i = 0; i < c1->num; i++)
    {
        (*covariance)[0] += (c1->points[i].x - c1->com.x) * (c2->points[i].x - c2->com.x);
        (*covariance)[1] += (c1->points[i].x - c1->com.x) * (c2->points[i].y - c2->com.y);
        (*covariance)[2] += (c1->points[i].y - c1->com.y) * (c2->points[i].x - c2->com.x);
        (*covariance)[3] += (c1->points[i].y - c1->com.y) * (c2->points[i].y - c2->com.y);
    }

    (*covariance)[0] /= c1->num;
    (*covariance)[1] /= c1->num;
    (*covariance)[2] /= c1->num;
    (*covariance)[3] /= c1->num;
}

void dot_product_2(double (*a)[4], double (*b)[4])
{
    (*a)[0] = (*a)[0] * (*b)[0];
    (*a)[1] = (*a)[1] * (*b)[1];
    (*a)[2] = (*a)[2] * (*b)[2];
    (*a)[3] = (*a)[3] * (*b)[3];
}

void multiply_2(double (*a)[4], double (*b)[4])
{
    (*a)[0] = (*a)[0] * (*b)[0] + (*a)[1] * (*b)[2];
    (*a)[1] = (*a)[0] * (*b)[1] + (*a)[1] * (*b)[3];
    (*a)[2] = (*a)[2] * (*b)[0] + (*a)[3] * (*b)[2];
    (*a)[3] = (*a)[2] * (*b)[1] + (*a)[3] * (*b)[3];
}

void copy_2(double (*a)[4], double (*b)[4])
{
    (*a)[0] = (*b)[0];
    (*a)[1] = (*b)[1];
    (*a)[2] = (*b)[2];
    (*a)[3] = (*b)[3];
}

void transpose_2(double (*a)[4])
{
    double temp[4] = {0};
    temp[0] = (*a)[0];
    temp[1] = (*a)[2];
    temp[2] = (*a)[1];
    temp[3] = (*a)[3];
    copy_2(a,&temp);
}

void svd_2(double (*input)[4], double (*u)[4], double (*sigma)[4], double (*v)[4])
{
    double y1 = input[1][0] + input[0][1];
    double y2 = input[1][0] - input[0][1];
    double x1 = input[0][0] - input[1][1];
    double x2 = input[0][0] + input[1][1];

    double h1 = sqrt(y1 * y1 + x1 * x1);
    double h2 = sqrt(y2 * y2 + x2 * x2);

    double t1 = x1 / h1;
    double t2 = x2 / h2;

    double cc = sqrt((1 + t1) * (1 + t2));
    double ss = sqrt((1 - t1) * (1 - t2));
    double cs = sqrt((1 + t1) * (1 - t2));
    double sc = sqrt((1 - t1) * (1 + t2));

    double c1 = (cc - ss) / 2;
    double s1 = (sc + cs) / 2;

    (*u)[0] = c1;
    (*u)[1] = -s1;
    (*u)[2] = s1;
    (*u)[3] = c1;

    (*sigma)[0] = (h1 + h2) / 2;
    (*sigma)[1] = 0;
    (*sigma)[2] = 0;
    (*sigma)[3] = (h1 - h2) / 2;

    if (h1 != h2)
    {
        (*v)[0] = 1 / (*sigma)[0];
        (*v)[1] = 0;
        (*v)[2] = 0;
        (*v)[3] = 1 / (*sigma)[3];
    }
    else
    {
        (*v)[0] = 1 / (*sigma)[0];
        (*v)[1] = 0;
        (*v)[2] = 0;
        (*v)[3] = 0;
    }

    double uT[4] = {0};
    copy_2(&uT,u);
    transpose_2(&uT);
    multiply_2(v, &uT);
    multiply_2(v, input);
}

void print_2(double (*input)[4])
{
    printf("[ %3f %3f ]\n[ %3f %3f ]\n", (*input)[0], (*input)[1], (*input)[2], (*input)[3]);
}

// transform mag_latch()
// {

// }

int main(int argc, char *argv[])
{
    SDL_Event event;
    int close;

    struct point cloud_a_points[16] = {
        {20, 20},
        {30, 30},
        {40, 40},
        {50, 50},
        {60, 60},
        {70, 70},
        {80, 80},
        {90, 90},
        {100, 100},
        {110, 110},
        {120, 120},
        {130, 130},

        {140, 120},
        {150, 110},
        {160, 100},
        {170, 90},
    };

    // int offset_x = rand_in_range(20, 500);
    // int offset_y = rand_in_range(20, 500);

    int offset_x = 200;
    int offset_y = 0;

    // struct point cloud_b_points[16] = {
    //     {20 + offset, 20},
    //     {30 + offset, 30},
    //     {40 + offset, 40},
    //     {50 + offset, 50},
    //     {60 + offset, 60},
    //     {70 + offset, 70},
    //     {80 + offset, 80},
    //     {90 + offset, 90},
    //     {100 + offset, 100},
    //     {110 + offset, 110},
    //     {120 + offset, 120},
    //     {130 + offset, 130},

    //     {140 + offset, 120},
    //     {150 + offset, 110},
    //     {160 + offset, 100},
    //     {170 + offset, 90},
    // };

    struct point cloud_b_points[16] = {
        {20 + offset_x, 20 + offset_y},
        {30 + offset_x, 30 + offset_y},
        {40 + offset_x, 40 + offset_y},
        {50 + offset_x, 50 + offset_y},
        {60 + offset_x, 60 + offset_y},
        {70 + offset_x, 70 + offset_y},
        {80 + offset_x, 80 + offset_y},
        {90 + offset_x, 90 + offset_y},
        {100 + offset_x, 100 + offset_y},
        {110 + offset_x, 110 + offset_y},
        {120 + offset_x, 120 + offset_y},
        {130 + offset_x, 130 + offset_y},

        {140 + offset_x, 120 + offset_y},
        {150 + offset_x, 110 + offset_y},
        {160 + offset_x, 100 + offset_y},
        {170 + offset_x, 90 + offset_y},
    };

    // struct point cloud_b_points[11] = {
    //     {70 + offset, 70},
    //     {80 + offset, 80},
    //     {90 + offset, 90},
    //     {100 + offset, 100},
    //     {110 + offset, 110},
    //     {120 + offset, 120},
    //     {130 + offset, 130},

    //     {140 + offset, 120},
    //     {150 + offset, 110},
    //     {160 + offset, 100},
    //     {170 + offset, 90},
    // };

    struct cloud *cloud_a = malloc(sizeof(cloud));
    struct cloud *cloud_b = malloc(sizeof(cloud));
    struct cloud *cloud_c = malloc(sizeof(cloud));
    cloud_init(cloud_a, cloud_a_points, sizeof(cloud_a_points) / sizeof(cloud_a_points[0]));
    cloud_init(cloud_b, cloud_b_points, sizeof(cloud_b_points) / sizeof(cloud_b_points[0]));
    cloud_init(cloud_c, cloud_b_points, sizeof(cloud_b_points) / sizeof(cloud_b_points[0]));

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

    // Loop control
    close = 0;

    // ICP control
    int icp = 0;
    double last_error = 0;
    while (!close)
    {

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
                icp = 1;
                break;
            }
        }

        // Process ICP physics-based (no rotation)
        if (icp)
        {
            // transform t;
            // t.a = 0;
            // t.x = 0;
            // t.y = 0;

            // // Determine transform
            // for (int i = 0; i < cloud_c->num; i++)
            // {
            //     point closest = closest_point(cloud_a, cloud_c->points[i]);
            //     t.x += closest.x - cloud_c->points[i].x;
            //     t.y += closest.y - cloud_c->points[i].y;
            //     printf("A:%f\n",angle_between_points(cloud_c->points[i],closest,cloud_c->com));
            //     // t.a +=angle_between_points(cloud_c->points[i],closest,cloud_c->com);
            // }
            // t.x /= cloud_c->num;
            // t.y /= cloud_c->num;
            // // t.a /= cloud_c->num*10;

            // // Overshoot
            // // t.x *= 1.5;
            // // t.y *= 1.5;

            // // Apply transform
            // apply_transform(cloud_c, t);

            // icp = 0;
            // double error = calculate_error(cloud_a, cloud_c);
            // printf("%f\n", error);
            // if (fabs(last_error - error) < 0.1)
            // {
            //     printf("Local minimum detected, annealing\n");
            //     double min_SA_error = error;
            //     transform best_t_SA = {0, 0, 0};
            //     for (double temp = 20; temp > 0; temp -= 0.1)
            //     {
            //         for (int i = 0; i < 100; i++)
            //         {
            //             cloud cloud_SA;
            //             cloud_SA.com = cloud_c->com;
            //             cloud_SA.num = cloud_c->num;
            //             cloud_SA.points = malloc(sizeof(point) * cloud_c->num);
            //             memcpy(cloud_SA.points, cloud_c->points, sizeof(point) * cloud_c->num);
            //             transform t_SA = best_t_SA;
            //             double velocity = 5;
            //             t_SA.x += rand_in_range(-velocity, velocity);
            //             t_SA.y += rand_in_range(-velocity, velocity);
            //             // printf("Transform x:%f y:%f\n", t_SA.x,  t_SA.y);
            //             apply_transform(&cloud_SA, t_SA);
            //             double error_SA = calculate_error(cloud_a, &cloud_SA);
            //             // printf("r:%f th:%f e:%f t:%f\n", rand_in_range(0, 1), thermal_energy(error_SA, temp), error_SA, temp);
            //             if (error_SA < min_SA_error || rand_in_range(0, 1) < thermal_energy(error_SA, temp))
            //             {
            //                 // printf("Error %f, %f\n", error_SA, min_SA_error);
            //                 min_SA_error = error_SA;
            //                 best_t_SA = t_SA;
            //             }
            //             free(cloud_SA.points);
            //         }
            //     }
            //     if (min_SA_error < error)
            //     {
            //         // printf("Better position found: x:%f, y:%f, a:%f\n", best_t_SA.x, best_t_SA.y, best_t_SA.a);
            //         apply_transform(cloud_c, best_t_SA);
            //     }
            //     else
            //     {
            //         // printf("No better position found.\n");
            //     }
            // }

            // printf("%f\n", error);
            // last_error = error;
        }

        // Process ICP matrix-based
        if (icp)
        {
            double covariance[4] = {0};
            double u[4] = {0};
            double sigma[4] = {0};
            double v[4] = {0};
            double rotation[4] = {0};
            
            // double rotation[4] = {0};
            // double rotation[4] = {-0.5625,0.8268, 0.8269, 0.5623};
            // double rotation[4] = {0.82633298,-0.56318187, 0.56318187, 0.82633298};
            // double rotation[4] = {0.3656523751389835, 0.9307515058381452, 0.9307515058381452, -0.3656523751389835};

            //              0.82633298 -0.56318187]
            //  [ 0.56318187  0.82633298
            compute_covariance(cloud_c, cloud_a, &covariance);
            printf("Input:\n");
            print_2(&covariance);

            svd_2(&covariance,&u,&sigma,&v);
            printf("U:\n");
            print_2(&u);
            printf("Sigma:\n");
            print_2(&sigma);
            printf("V:\n");
            print_2(&v);

            copy_2(&rotation,&u);
            multiply_2(&rotation,&v);
            printf("Rotation:\n");
            print_2(&rotation);

            // rotate_cloud_matrix(cloud_c, &rotation);
            icp = 0;
        }

        // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
        //                       BEGIN GRAPHICS
        // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

        // Process the screen
        SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
        SDL_RenderClear(rend);

        // Draw point cloud a
        SDL_SetRenderDrawColor(rend, 0, 255, 0, 255);
        for (int i = 0; i < cloud_a->num; i++)
        {
            // printf("%d\n",i);
            DrawCircle(rend, cloud_a->points[i].x, cloud_a->points[i].y, 3);
        }
        SDL_SetRenderDrawColor(rend, 255, 255, 0, 255);
        DrawCircle(rend, cloud_a->com.x, cloud_a->com.y, 2);

        // Draw point cloud b
        SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
        for (int i = 0; i < cloud_b->num; i++)
        {
            // printf("%d\n",i);
            DrawCircle(rend, cloud_b->points[i].x, cloud_b->points[i].y, 3);
        }
        SDL_SetRenderDrawColor(rend, 255, 100, 0, 255);
        DrawCircle(rend, cloud_b->com.x, cloud_b->com.y, 2);

        // Draw point cloud c
        SDL_SetRenderDrawColor(rend, 0, 50, 255, 255);
        for (int i = 0; i < cloud_b->num; i++)
        {
            // printf("%d\n",i);
            DrawCircle(rend, cloud_c->points[i].x, cloud_c->points[i].y, 3);
            point closest = closest_point(cloud_a, cloud_c->points[i]);
            SDL_RenderDrawLine(rend, cloud_c->points[i].x, cloud_c->points[i].y, closest.x, closest.y);
        }
        SDL_SetRenderDrawColor(rend, 255, 100, 0, 255);
        DrawCircle(rend, cloud_c->com.x, cloud_c->com.y, 2);

        SDL_RenderPresent(rend);

        // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
        //                       END GRAPHICS
        // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
    }
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}
