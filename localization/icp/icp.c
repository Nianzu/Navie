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
        c->points[i].y += y;
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
    (*out) = (*input)[0] * (*input)[3] - (*input)[1] * (*input)[2];
}

// Get the covariance of c1 relative to c2
void compute_covariance(cloud *c1, cloud *c2, double (*covariance)[4])
{

    for (int i = 0; i < c1->num; i++)
    {
        // printf("x:%f y:%f\n",c2->points[i].x,c2->com.x);
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
    copy_2(a, &temp);
}

void transpose_2(double (*a)[4])
{
    double temp[4] = {0};
    temp[0] = (*a)[0];
    temp[1] = (*a)[2];
    temp[2] = (*a)[1];
    temp[3] = (*a)[3];
    copy_2(a, &temp);
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
    copy_2(&uT, u);
    transpose_2(&uT);
    cross_product_2(v, &uT);
    cross_product_2(v, input);
}

int eigenvalues_2(double (*input)[4], double (*e)[4])
{

    // a b
    // c d
    // det(A - LI) = (a-L)(d-L) - bc
    // = L^2 - (a+d)L + ad-bc

    // x = (-b +- sqrt(b^2 - 4ac))/2a

    double a = 1;
    double b = -(*input)[0] - (*input)[3];
    double c = (*input)[0] * (*input)[3] - (*input)[1] * (*input)[2];
    double discriminant = b * b - 4 * a * c;
    (*e)[1] = (*e)[2] = 0;

    // Real eigenvalues
    if (discriminant >= 0)
    {
        (*e)[0] = (-b + sqrt(discriminant)) / (2 * a);
        (*e)[3] = (-b - sqrt(discriminant)) / (2 * a);
        return 1;
    }
    // Complex eigenvalues
    else
    {
        printf("COMPLEX EIGENVALUES");
        return 0;
    }
}

void eigenvectors_2(double (*input)[4], double (*sigma)[4], double (*ev)[4])
{
    double a = (*input)[0], b = (*input)[1], c = (*input)[2], d = (*input)[3];
    double lambda1 = (*sigma)[0], lambda2 = (*sigma)[3];
    double v1_x1, v1_x2, v2_x1, v2_x2;
    // [ a-L  b   ]
    // [ c    d-L ]

    // Compute for Lambda 1
    if ((b == 0 && a - lambda1 != 0) || (d - lambda1 == 0 && c != 0))
    {
        (*ev)[0] = 0.0;
    }
    else
    {
        (*ev)[0] = 1.0;
    }
    if ((b != 0 && a - lambda1 == 0) || (d - lambda1 != 0 && c == 0))
    {
        (*ev)[2] = 0;
    }
    if ((*ev)[0] == 0)
    {
        (*ev)[2] = 1.0;
    }
    else
    {
        (*ev)[2] = (lambda1 - a) / b;
    }

    // Compute for lambda 2
    if ((b == 0 && a - lambda2 != 0) || (d - lambda2 == 0 && c != 0))
    {
        (*ev)[1] = 0.0;
    }
    else
    {
        (*ev)[1] = 1.0;
    }
    if ((b != 0 && a - lambda2 == 0) || (d - lambda2 != 0 && c == 0))
    {
        (*ev)[3] = 0;
    }
    if ((*ev)[1] == 0)
    {
        (*ev)[3] = 1.0;
    }
    else
    {
        (*ev)[3] = (lambda2 - a) / b;
    }
}

void eigenvectors_orthonormal_2(double (*input)[4], double (*sigma)[4], double (*ev)[4])
{
    eigenvectors_2(input, sigma, ev);
    double h1 = sqrt((*ev)[0] * (*ev)[0] + (*ev)[2] * (*ev)[2]);
    double h2 = sqrt((*ev)[1] * (*ev)[1] + (*ev)[3] * (*ev)[3]);
    (*ev)[0] /= h1;
    (*ev)[1] /= h2;
    (*ev)[2] /= h1;
    (*ev)[3] /= h2;
}

void inverse_2(double (*input)[4])
{
    double det;
    determinant_2(input, &det);
    double temp = (*input)[0];
    (*input)[0] = (*input)[3];
    (*input)[3] = temp;
    (*input)[1] *= -1;
    (*input)[2] *= -1;

    (*input)[0] /= det;
    (*input)[1] /= det;
    (*input)[2] /= det;
    (*input)[3] /= det;
}

void svd_2_new(double (*a)[4], double (*u)[4], double (*sigma)[4], double (*vt)[4])
{
    double at[4];
    double ata[4];
    double eigenvalues[4];
    double eigenvectors[4];
    double sigma_inv[4];
    double v[4];

    // Get AT * A
    copy_2(&at, a);
    transpose_2(&at);
    copy_2(&ata, &at);
    cross_product_2(&ata, a);

    // Get eigenvalues (sigma matrix)
    eigenvalues_2(&ata, &eigenvalues);
    (*sigma)[0] = sqrt(eigenvalues[0]);
    (*sigma)[1] = 0;
    (*sigma)[2] = 0;
    (*sigma)[3] = sqrt(eigenvalues[3]);

    // Get eigenvectors
    eigenvectors_orthonormal_2(&ata, &eigenvalues, &v);
    copy_2(vt, &v);
    transpose_2(vt);

    // Get U vector
    copy_2(&sigma_inv, sigma);
    inverse_2(&sigma_inv);
    copy_2(u, a);
    cross_product_2(u, &v);
    cross_product_2(u, &sigma_inv);
}

void print_2(double (*input)[4])
{
    printf("[ %3f %3f ]\n[ %3f %3f ]\n", (*input)[0], (*input)[1], (*input)[2], (*input)[3]);
}

void icp(cloud *cloud_a, cloud *cloud_c)
{
    double covariance[4] = {0};
    double u[4] = {0};
    double sigma[4] = {0};
    double v[4] = {0};
    double rotation[4] = {0};
    double covariance_check[4] = {0};

    translate_cloud(cloud_c, cloud_a->com.x - cloud_c->com.x, cloud_a->com.y - cloud_c->com.y);
    compute_covariance(cloud_c, cloud_a, &covariance);

    svd_2_new(&covariance, &u, &sigma, &v);

    copy_2(&rotation, &u);
    cross_product_2(&rotation, &v);
    transpose_2(&rotation);
    rotate_cloud_matrix(cloud_c, &rotation);
}

void icp2(cloud *cloud_a, cloud *cloud_b)
{
    double covariance[4] = {0};
    double u[4] = {0};
    double sigma[4] = {0};
    double v[4] = {0};
    double rotation[4] = {0};
    double covariance_check[4] = {0};
    cloud cloud_c;
    cloud_c.points = malloc(sizeof(point)*cloud_b->num);
    
    translate_cloud(cloud_b, cloud_a->com.x - cloud_b->com.x, cloud_a->com.y - cloud_b->com.y);

    cloud_c.com.x = 0;
    cloud_c.com.y = 0;
    cloud_c.num = cloud_b->num;
    for (int i = 0; i < cloud_b->num; i++)
    {
        point p = closest_point(cloud_a,cloud_b->points[i]);
        cloud_c.com.x += p.x;
        cloud_c.com.y += p.y;
        cloud_c.points[i].x = p.x;
        cloud_c.points[i].y = p.y;
    }
    cloud_c.com.x /= cloud_c.num;
    cloud_c.com.y /= cloud_c.num;
    // printf("x:%f y:%f\n", cloud_a->com.x - cloud_b->com.x, cloud_a->com.y - cloud_b->com.y);
    compute_covariance(cloud_b, &cloud_c, &covariance);
    print_2(&covariance);

    svd_2_new(&covariance, &u, &sigma, &v);

    copy_2(&rotation, &u);
    cross_product_2(&rotation, &v);
    transpose_2(&rotation);
    print_2(&rotation);
    // rotate_cloud_matrix(cloud_b, &rotation);
}