#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#include <unistd.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define PORT 12345

typedef struct {
    int x;
    int y;
} Point;


int min_x = INT_MAX, min_y = INT_MAX;
int max_x = INT_MIN, max_y = INT_MIN;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

Point *points = NULL;  // Dynamically allocated array of points
int point_count = 0;   // Number of points stored

int init_sdl() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    window = SDL_CreateWindow("Robot Path", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    return 0;
}

void add_point(int x, int y) {
    Point *new_points = realloc(points, (point_count + 1) * sizeof(Point));
    if (new_points == NULL) {
        perror("Failed to allocate memory");
        free(points);
        exit(EXIT_FAILURE);
    }
    points = new_points;
    points[point_count].x = x;
    points[point_count].y = y;
    point_count++;

    if (x < min_x) min_x = x;
    if (x > max_x) max_x = x;
    if (y < min_y) min_y = y;
    if (y > max_y) max_y = y;
}

double scale(double coordinate, double min_input, double max_input, double min_output, double max_output) {
    return ((coordinate - min_input) / (max_input - min_input)) * (max_output - min_output) + min_output;
}

void draw_points() {
    int margin = 10;

    for (int i = 0; i < point_count; i++) {

    }

    int max_x_local = max_x + margin;
    int min_x_local = min_x - margin;
    int max_y_local = max_y + margin;
    int min_y_local = min_y - margin;

    int range_x = max_x_local - min_x_local;
    int range_y = max_y_local - min_y_local;
    // float scale_x = (range_x > 0) ? WINDOW_WIDTH / (float)range_x : 0;
    // float scale_y = (range_y > 0) ? WINDOW_HEIGHT / (float)range_y : 0;
    // float scale = (scale_x > scale_y) ? scale_x : scale_y;
    // printf("rangex: %d rangey: %d\n",range_x,range_y);
    int range = (range_x > range_y) ? range_x : range_y;
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

    int prev_x = 0;
    int prev_y = 0;

    for (int i = 0; i < point_count; i++) {
        // int render_x = ((points[i].x - min_x) * scale);
        // int render_y = ((points[i].y - min_y) * scale);
        int render_x = scale(points[i].x,min_x_local,max_x_local,0,WINDOW_WIDTH);
        int render_y = scale(points[i].y,min_y_local,max_y_local,0,WINDOW_HEIGHT);

        // int render_x = (points[i].x + min_x_local) * WINDOW_WIDTH / range_x + WINDOW_WIDTH/2;
        // int render_y = (points[i].y + min_y_local) * WINDOW_HEIGHT / range_y + WINDOW_HEIGHT/2;
        if (i!= 0)
        {
            SDL_RenderDrawLine(renderer, render_x, render_y,prev_x,prev_y);
        }
        
        // SDL_RenderDrawPoint(renderer, render_x, render_y);
        prev_x = render_x;
        prev_y = render_y;
    }

    SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[]) {
    if (init_sdl() != 0) {
        return 1;
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in servaddr, cliaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    socklen_t len;
    char buffer[1024];
    int x, y;
    SDL_Event e;

    while (1) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(window);
                SDL_Quit();
                close(sockfd);
                free(points);
                return 0;
            }
        }

        int n = recvfrom(sockfd, buffer, 1024, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);
        buffer[n] = '\0';
        sscanf(buffer, "%d %d", &x, &y);
        add_point(x, y);
        draw_points();
    }

    return 0;
}
