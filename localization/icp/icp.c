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

void translate_cloud(cloud *c, double x, double y)
{
    for (int i = 0; i < c->num; i++)
    {
        c->points[i].x += x;
        c->points[i].x += y;
    }
    c->com.x += x;
    c->com.y += y;
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

void determinant_2(double (*input)[4], double *out)
{
    (*out) = (*input)[0]* (*input)[3] - (*input)[1]* (*input)[2];
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

void copy_2(double (*a)[4], double (*b)[4])
{
    (*a)[0] = (*b)[0];
    (*a)[1] = (*b)[1];
    (*a)[2] = (*b)[2];
    (*a)[3] = (*b)[3];
}

void cross_product_2(double (*a)[4], double (*b)[4])
{
    double temp[4] = {0};
    temp[0] = (*a)[0] * (*b)[0] + (*a)[1] * (*b)[2];
    temp[1] = (*a)[0] * (*b)[1] + (*a)[1] * (*b)[3];
    temp[2] = (*a)[2] * (*b)[0] + (*a)[3] * (*b)[2];
    temp[3] = (*a)[2] * (*b)[1] + (*a)[3] * (*b)[3];
    copy_2(a,&temp);
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
    double y1 = (*input)[2] + (*input)[1];
    double y2 = (*input)[2] - (*input)[1];
    double x1 = (*input)[0] - (*input)[3];
    double x2 = (*input)[0] + (*input)[3];

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
    (*sigma)[3] = fabs(h1 - h2) / 2;

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
    cross_product_2(v, &uT);
    cross_product_2(v, input);
}

void print_2(double (*input)[4])
{
    printf("[ %3f %3f ]\n[ %3f %3f ]\n", (*input)[0], (*input)[1], (*input)[2], (*input)[3]);
}