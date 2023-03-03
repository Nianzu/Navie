// Nico Zucca, 1/2023

// Compile cmd:
// export LD_LIBRARY_PATH="/usr/local/lib"
// gcc main.c -o main.o `sdl2-config --cflags --libs` -lm -O3

#include "icp.c"

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
    double rot = 0.1;
    // double rot = rand_in_range(0, M_PI * 2);
    rotate_cloud(cloud_c, rot);
    rotate_cloud(cloud_b, rot);

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
            translate_cloud(cloud_c,cloud_a->com.x -cloud_c->com.x,cloud_a->com.y -cloud_c->com.y);
            compute_covariance(cloud_c, cloud_a, &covariance);
            printf("Input:\n");
            print_2(&covariance);

            svd_2(&covariance, &u, &sigma, &v);
            printf("U:\n");
            print_2(&u);
            printf("Sigma:\n");
            print_2(&sigma);
            printf("V:\n");
            print_2(&v);

            copy_2(&rotation, &u);
            // cross_product_2(&rotation, &sigma);
            cross_product_2(&rotation, &v);
            transpose_2(&rotation);
            printf("Rotation:\n");
            print_2(&rotation);
            double det;
            determinant_2(&rotation, &det);
            printf("Det: %f\n", det);

            rotate_cloud_matrix(cloud_c, &rotation);

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
