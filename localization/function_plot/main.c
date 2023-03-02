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

double func_gradient(int x, int y)
{
    return (double)y / (double)WINDOW_WIDTH;
}

double func_gausian(int x, int y)
{
    return 1 / (1 + pow(sqrt(pow(x, 2) + pow(y, 2)) / 20, 2));
}

double func_gausian_elipse(int x, int y)
{
    return 1 / (1 + pow(sqrt(pow(x / 2, 2) + pow(y, 2)) / 20, 2));
}

double func_gausian_bannana(int x, int y)
{
    int y_new = y + sqrt(fmax(pow(200, 2) - pow(x, 2), 0.0));
    return 1 / (1 + pow(sqrt(pow(x / 2, 2) + pow(y_new, 2)) / 20, 2));
}

double func_anime_gausian_bannana(int x, int y, double a)
{
    int y_new = y - a + sqrt(fmax(pow(a, 2) - pow(x, 2), 0.0));
    return 1 / (1 + pow(sqrt(pow(x / 2, 2) + pow(y_new, 2)) / (a / 15), 2));
}

double func_anime_gausian_bannana2(double x, double y, double robot_x, double robot_y, double radius, double angle)
{
    double new_angle = (M_PI * 2) - angle;
    double x_new = x - robot_x;
    double y_new = y - robot_y;

    double x_rot = x_new * cosf(new_angle) - y_new * sinf(new_angle);
    double y_rot = x_new * sinf(new_angle) + y_new * cosf(new_angle);

    y_rot += -radius + sqrt(fmax(pow(radius, 2) - pow(x_rot, 2), 0.0));
    return 1 / (1 + pow(sqrt(pow(x_rot / 2, 2) + pow(y_rot, 2)) / (radius / 15), 2));
}

double func_anime_gausian_bannana3(double x, double y, double robot_x, double robot_y, double radius, double dist, double angle)
{
    double new_angle = (M_PI * 2) - angle;
    double x_new = x - robot_x;
    double y_new = y - robot_y;

    double x_rot = x_new * cosf(new_angle) - y_new * sinf(new_angle);
    double y_rot = x_new * sinf(new_angle) + y_new * cosf(new_angle);

    y_rot += -radius + sqrt(fmax(pow(radius, 2) - pow(x_rot, 2), 0.0));
    return 1 / (1 + pow(sqrt(pow(x_rot / 2, 2) + pow(y_rot, 2)) / (dist / 15), 2));
}

int main(int argc, char *argv[])
{
    SDL_Event event;
    int close;

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
    double a = 0;
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

                break;
            }
        }

        // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
        //                       BEGIN GRAPHICS
        // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

        // Process the screen
        SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
        SDL_RenderClear(rend);

        // Draw the function
        for (int i = 0; i < WINDOW_WIDTH; i++)
        {
            for (int j = 0; j < WINDOW_HEIGHT; j++)
            {
                // SDL_SetRenderDrawColor(rend, 0, clamp_and_scale(0, 1, 255, func_gradient(i - WINDOW_WIDTH / 2, j - WINDOW_HEIGHT / 2)), 0, 255);
                // SDL_SetRenderDrawColor(rend, 0, clamp_and_scale(0, 1, 255, func_gausian(i - WINDOW_WIDTH / 2, j - WINDOW_HEIGHT / 2)), 0, 255);
                // SDL_SetRenderDrawColor(rend, 0, clamp_and_scale(0, 1, 255, func_gausian_elipse(i - WINDOW_WIDTH / 2, j - WINDOW_HEIGHT / 2)), 0, 255);
                // SDL_SetRenderDrawColor(rend, 0, clamp_and_scale(0, 1, 255, func_anime_gausian_bannana(i - WINDOW_WIDTH / 2, j - WINDOW_HEIGHT / 2,clamp_and_scale(0,M_PI*2,200,a))), 0, 255);
                // SDL_SetRenderDrawColor(rend, 0, clamp_and_scale(0, 1, 255, func_anime_gausian_bannana2(i, j,WINDOW_WIDTH / 2,WINDOW_HEIGHT / 2,200,a)), 0, 255);
                SDL_SetRenderDrawColor(rend, 0, clamp_and_scale(0, 1, 255, func_anime_gausian_bannana3(i, j,WINDOW_WIDTH / 2,WINDOW_HEIGHT / 2,clamp_and_scale(0,M_PI*2,400,a)-200,200,0)), 0, 255);
                SDL_RenderDrawPoint(rend, i, j);
            }
        }

        // Draw rotation indicator
        // SDL_SetRenderDrawColor(rend, 0, 255, 255, 255);
        // double x_rot = 0 * cosf(a) - 50 * sinf(a);
        // double y_rot = 0 * sinf(a) + 50 * cosf(a);
        // SDL_RenderDrawLine(rend, x_rot + WINDOW_WIDTH / 2, y_rot + WINDOW_HEIGHT / 2, x_rot * 2 + WINDOW_WIDTH / 2, y_rot * 2 + WINDOW_HEIGHT / 2);

        // Draw the origin
        SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
        int len = 10;
        SDL_RenderDrawLine(rend, (WINDOW_WIDTH / 2) + len, WINDOW_HEIGHT / 2, (WINDOW_WIDTH / 2) - len, WINDOW_HEIGHT / 2);
        SDL_RenderDrawLine(rend, WINDOW_WIDTH / 2, (WINDOW_HEIGHT / 2) + len, WINDOW_WIDTH / 2, (WINDOW_HEIGHT / 2) - len);

        SDL_RenderPresent(rend);

        a += 0.1;
        if (a > M_PI * 2)
        {
            a = 0;
        }

        // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
        //                       END GRAPHICS
        // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
    }
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}
