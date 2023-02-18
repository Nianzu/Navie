// Nico Zucca, 12/2022

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <time.h>
#include <string.h>

#define KERNEL_EDGE_SIZE 5      // How many pixels above/below/left/right of given pixel to SAD for disparity metric. Default 5
#define BLOCK_SIZE 20           // How many pixels right of given to check for disparity metric. Default 50
#define RGB_COMPONENT_COLOR 255 // Used for exporting the ppm file

// Compile cmd:
// gcc -g main.c -o main.o

struct PPMPixel
{
    unsigned char red, green, blue;
};

struct hsv_pixel
{
    double hue, saturation, value;
};

struct PPMImage
{
    int x, y;
    struct PPMPixel *data;
};

struct ppm_array
{
    int height;
    int width;
    struct PPMPixel ***arr;
};

struct disparity_map
{
    int height;
    int width;
    double **arr;
};

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

// https://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
struct PPMPixel hsv_to_rgb(struct hsv_pixel in)
{
    double hh, p, q, t, ff, v;
    long i;
    struct PPMPixel out;

    v = clamp_and_scale(0, 1, 255, in.value);

    if (in.saturation <= 0.0)
    { // < is bogus, just shuts up warnings
        out.red = v;
        out.green = v;
        out.blue = v;
        return out;
    }
    hh = in.hue;
    if (hh >= 360.0)
        hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = clamp_and_scale(0, 1, 255, in.value * (1.0 - in.saturation));
    q = clamp_and_scale(0, 1, 255, in.value * (1.0 - (in.saturation * ff)));
    t = clamp_and_scale(0, 1, 255, in.value * (1.0 - (in.saturation * (1.0 - ff))));

    switch (i)
    {
    case 0:
        out.red = in.value;
        out.green = t;
        out.blue = p;
        break;
    case 1:
        out.red = q;
        out.green = in.value;
        out.blue = p;
        break;
    case 2:
        out.red = p;
        out.green = in.value;
        out.blue = t;
        break;

    case 3:
        out.red = p;
        out.green = q;
        out.blue = in.value;
        break;
    case 4:
        out.red = t;
        out.green = p;
        out.blue = in.value;
        break;
    case 5:
    default:
        out.red = in.value;
        out.green = p;
        out.blue = q;
        break;
    }

    // printf("HSV:%3f,%3f,%3f RGB:%d,%d,%d\n",in.hue,in.saturation,in.value,out.red,out.blue,out.green);
    return out;
}

// Allocates space for the ppm_array, assumes that the height and width members have been set and are correct.
void ppm_array_allocate(struct ppm_array *obj)
{
    (*obj).arr = (struct PPMPixel ***)malloc(sizeof(struct PPMPixel **) * (*obj).width);
    for (int x = 0; x < (*obj).width; x++)
    {
        (*obj).arr[x] = (struct PPMPixel **)malloc(sizeof(struct PPMPixel *) * (*obj).height);
        for (int y = 0; y < (*obj).height; y++)
        {
            (*obj).arr[x][y] = (struct PPMPixel *)malloc(sizeof(struct PPMPixel));
        }
    }
}

// Allocates space for the disparity map, assumes that the height and width members have been set and are correct.
void allocate_disparity_map(struct disparity_map *obj)
{
    (*obj).arr = (double **)malloc(sizeof(double *) * (*obj).width);
    for (int x = 0; x < (*obj).width; x++)
    {
        (*obj).arr[x] = (double *)malloc(sizeof(double) * (*obj).height);
        // for (int y = 0; y < (*obj).height; y++)
        // {
        //     (*obj).arr[x][y] = (struct PPMPixel *)malloc(sizeof(struct PPMPixel));
        // }
    }
}

// Frees space from the ppm_array.
void free_ppm_array(struct ppm_array *obj)
{
    for (int x = 0; x < (*obj).width; x++)
    {
        for (int y = 0; y < (*obj).height; y++)
        {
            free((*obj).arr[x][y]);
        }
        free((*obj).arr[x]);
    }
    free((*obj).arr);
}

void free_disparity_map(struct disparity_map *obj)
{
    for (int x = 0; x < (*obj).width; x++)
    {
        // for (int y = 0; y < (*obj).height; y++)
        // {
        //     free((*obj).arr[x][y]);
        // }
        free((*obj).arr[x]);
    }
    free((*obj).arr);
}

// Converts the loaded image buffer to an array for easier use.
void img_to_arr(struct PPMImage *img, struct ppm_array *obj)
{
    int i;
    int x;
    int y;
    x = y = 0;
    (*obj).height = img->y;
    (*obj).width = img->x;
    ppm_array_allocate(obj);
    for (i = 0; i < img->x * img->y; i++)
    {

        (*obj).arr[x][y]->red = img->data[i].red;
        (*obj).arr[x][y]->green = img->data[i].green;
        (*obj).arr[x][y]->blue = img->data[i].blue;
        x++;
        if (x >= img->x)
        {
            x = 0;
            y++;
        }
    }
}

// Converts the array back into an image, for viewing.
void arr_to_img(struct ppm_array *obj, struct PPMImage *img)
{
    int i;
    int x;
    int y;
    x = y = 0;
    img->x = obj->width;
    img->y = obj->height;
    for (i = 0; i < img->x * img->y; i++)
    {
        img->data[i].red = obj->arr[x][y]->red;
        img->data[i].green = obj->arr[x][y]->green;
        img->data[i].blue = obj->arr[x][y]->blue;
        x++;
        if (x >= img->x)
        {
            x = 0;
            y++;
        }
    }
}

int get_max_disparity(struct disparity_map *obj)
{
    int max = 0;
    for (int j = 0; j < obj->height; j++)
    {
        for (int i = 0; i < obj->width; i++)
        {
            if (obj->arr[i][j] > max)
            {
                max = obj->arr[i][j];
            }
        }
    }
    return max;
}

// Converts the array back into an image, for viewing.
// TODO no protection for writing a bigger file than is malloced
void disparity_map_to_img(struct disparity_map *obj, struct PPMImage *img)
{
    int i;
    int x;
    int y;
    x = y = 0;
    img->x = obj->width;
    img->y = obj->height;
    int max_disparity = get_max_disparity(obj);
    // printf("x,y %d,%d\n",img->x,img->y);
    for (i = 0; i < img->x * img->y; i++)
    {
        int newval = clamp_and_scale(0, max_disparity, 255, obj->arr[x][y]);
        // printf("i,max,val %d,%d,%d\n",i,img->x * img->y,newval);
        img->data[i].red =
            img->data[i].green =
                img->data[i].blue = newval;
        x++;
        if (x >= img->x)
        {
            x = 0;
            y++;
        }
    }
}

// Reads a PPM file into a buffer.
// Source: https://stackoverflow.com/questions/2693631/read-ppm-file-and-store-it-in-an-array-coded-with-c
// void readPPM(const char *filename, ppm_array *obj)
static struct PPMImage *readPPM(const char *filename)
{
    char buff[16];
    struct PPMImage *img;
    FILE *fp;
    int c, rgb_comp_color;
    // open PPM file for reading
    fp = fopen(filename, "rb");
    if (!fp)
    {
        fprintf(stderr, "Unable to open file '%s'\n", filename);
        exit(1);
    }

    // read image format
    if (!fgets(buff, sizeof(buff), fp))
    {
        perror(filename);
        exit(1);
    }

    // check the image format
    if (buff[0] != 'P' || buff[1] != '6')
    {
        fprintf(stderr, "Invalid image format (must be 'P6')\n");
        exit(1);
    }

    // alloc memory form image
    img = (struct PPMImage *)malloc(sizeof(struct PPMImage));
    if (!img)
    {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(1);
    }

    // check for comments
    c = getc(fp);
    while (c == '#')
    {
        while (getc(fp) != '\n')
            ;
        c = getc(fp);
    }

    ungetc(c, fp);
    // read image size information
    if (fscanf(fp, "%d %d", &img->x, &img->y) != 2)
    {
        fprintf(stderr, "Invalid image size (error loading '%s')\n", filename);
        exit(1);
    }

    // read rgb component
    if (fscanf(fp, "%d", &rgb_comp_color) != 1)
    {
        fprintf(stderr, "Invalid rgb component (error loading '%s')\n", filename);
        exit(1);
    }

    // check rgb component depth
    if (rgb_comp_color != RGB_COMPONENT_COLOR)
    {
        fprintf(stderr, "'%s' does not have 8-bits components\n", filename);
        exit(1);
    }

    while (fgetc(fp) != '\n')
        ;
    // memory allocation for pixel data
    img->data = (struct PPMPixel *)malloc(img->x * img->y * sizeof(struct PPMPixel));

    if (!img)
    {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(1);
    }

    // read pixel data from file
    if (fread(img->data, 3 * img->x, img->y, fp) != img->y)
    {
        fprintf(stderr, "Error loading image '%s'\n", filename);
        exit(1);
    }

    fclose(fp);
    return img;
}

// Prints the image to the cmd line, 1 pixel at a time. For debugging.
void print_img(struct PPMImage *img)
{
    printf("x,y : %d,%d\n", img->x, img->y);
    int i;
    for (i = 0; i < img->x * img->y; i++)
    {
        int x = i % img->x;
        int y = i / img->y;
        printf("x: %d, y: %d,  r: %d,  g: %d,  b: %d\n", x, y, img->data[i].red, img->data[i].green, img->data[i].blue);
    }
}

// Prints the array image to the command line, 1 pixel at a time. For debugging.
void print_arr(struct ppm_array obj)
{
    printf("width,height : %d,%d\n", obj.width, obj.height);
    for (int y = 0; y < obj.height; y++)
    {
        for (int x = 0; x < obj.width; x++)
        {
            // printf("x: %d, y: %d,  r: %d,  g: %d,  b: %d\n", x, y, obj.arr[x][y]->red, obj.arr[x][y]->green, obj.arr[x][y]->blue);
        }
    }
}

void writePPM(const char *filename, struct PPMImage *img)
{
    FILE *fp;
    // open file for output
    fp = fopen(filename, "wb");
    if (!fp)
    {
        fprintf(stderr, "Unable to open file '%s'\n", filename);
        exit(1);
    }

    // write the header file
    // image format
    fprintf(fp, "P6\n");

    // image size
    fprintf(fp, "%d %d\n", img->x, img->y);

    // rgb component depth
    fprintf(fp, "%d\n", RGB_COMPONENT_COLOR);

    // pixel data
    fwrite(img->data, 3 * img->x, img->y, fp);
    fclose(fp);
}

void to_greyscale(struct ppm_array *obj)
{
    // Needs to be in this order for the image to be output correctly
    for (int x = 0; x < (*obj).width; x++)
    {
        for (int y = 0; y < (*obj).height; y++)
        {
            int greyscale = 0;
            greyscale += (*obj).arr[x][y]->red;
            greyscale += (*obj).arr[x][y]->green;
            greyscale += (*obj).arr[x][y]->blue;
            greyscale /= 3;
            (*obj).arr[x][y]->red =
                (*obj).arr[x][y]->green =
                    (*obj).arr[x][y]->blue = greyscale;
        }
    }
}

int abs(int in)
{
    if (in < 0)
    {
        return -in;
    }
    return in;
}

// https://www.geeksforgeeks.org/inline-function-in-c/
int pixel_dif_abs(int x_1, int y_1, int x_2, int y_2, struct ppm_array *img_left, struct ppm_array *img_right)
{
    // Return -1 if out of bounds, so that we can process out of bounds differently
    if (x_1 < 0 || x_2 < 0 || y_1 < 0 || y_2 < 0 ||
        x_1 >= img_left->width || x_2 >= img_right->width || y_1 >= img_right->width || y_2 >= img_right->height)
    {
        // printf("OOB: x1 %d y1 %d x2 %d y2 %d 1c %d 1r %d 2c %d 2r %d\n",x_1,y_1,x_2,y_2,image_1.cols,image_1.rows,image_2.cols,image_2.rows);
        return -1;
    }
    int dif = 0;
    dif += abs(img_left->arr[x_1][y_1]->red - img_right->arr[x_2][y_2]->red);
    dif += abs(img_left->arr[x_1][y_1]->green - img_right->arr[x_2][y_2]->green);
    dif += abs(img_left->arr[x_1][y_1]->blue - img_right->arr[x_2][y_2]->blue);
    return dif;
}

double get_sum_absolute_difference(int x_1, int y_1, int x_2, int y_2, struct ppm_array *img_left, struct ppm_array *img_right)
{
    // Sum of squared differences
    double SAD = 0;
    int pixels = 0;
    for (int i = -KERNEL_EDGE_SIZE; i <= KERNEL_EDGE_SIZE; i++)
    {
        for (int j = -KERNEL_EDGE_SIZE; j <= KERNEL_EDGE_SIZE; j++)
        {
            int diff = pixel_dif_abs(x_1 + i, y_1 + j, x_2 + i, y_2 + j, img_left, img_right);
            if (diff != -1)
            {
                pixels++;
                SAD += diff;
            }
        }
    }

    // Divide by the number of valid pixels, to get average match
    SAD = SAD / (double)pixels;
    return SAD;
}

double parabolic_approximation(double C_1, double C_2, double C_3, double d_2)
{
    return d_2 - ((C_3 - C_1) / (C_1 - 2 * C_2 + C_3)) / 2;
}

double get_disparity(struct ppm_array *img_left, struct ppm_array *img_right, int x, int y, int search_len, int offset)
{
    double min_SAD = DBL_MAX;
    int disparity = 0;
    double disparity_f;
    double *disparity_map = calloc(search_len + 1, sizeof(double));
    for (int i = 0; -i <= search_len && i + x + KERNEL_EDGE_SIZE - offset > 0; i--)
    {
        // printf("i:%d x:%d KERNEL_EDGE_SIZE:%d img_left->width:%d\n",i,x,KERNEL_EDGE_SIZE,img_left->width);
        double new_SAD = get_sum_absolute_difference(x, y, x + i - offset, y, img_left, img_right);
        if (new_SAD < min_SAD)
        {
            min_SAD = new_SAD;
            disparity = -i + offset;
        }
        // printf("i:%d\n cont:%s",-i,-i <= BLOCK_SIZE ? "yes" : "no");
        disparity_map[-i] = new_SAD;
    }

    // Sub-pixel approximation
    // http://mccormickml.com/2014/01/10/stereo-vision-tutorial-part-i/
    if (disparity > 0 && disparity < search_len)
    {
        return parabolic_approximation(disparity_map[disparity - 1 - offset], disparity_map[disparity - offset], disparity_map[disparity + 1 - offset], disparity + offset);
    }
    return disparity;
}

void block_match(struct ppm_array *img_left, struct ppm_array *img_right, struct disparity_map *img_out, int search_len)
{
    for (int j = 0; j < img_out->height; j++)
    {
        for (int i = 0; i < img_out->width; i++)
        {

            img_out->arr[i][j] = get_disparity(img_left, img_right, i, j, search_len, 0);
        }
        printf("\r%d/%d - %2.0f%%", j, img_out->height, 100 * (double)j / (double)img_out->height);
        fflush(stdout);
    }
    printf("\n");
}

void block_match_informed(struct ppm_array *img_left, struct ppm_array *img_right, struct disparity_map *coarse, struct disparity_map *img_out, int search_len)
{
    for (int j = 0; j < img_out->height; j++)
    {
        for (int i = 0; i < img_out->width; i++)
        {
            int coarse_x = coarse->arr[i / 2][j / 2];
            img_out->arr[i][j] = get_disparity(img_left, img_right, i, j, search_len + 4, coarse_x - 1);
        }
        printf("\r%d/%d - %2.0f%%", j, img_out->height, 100 * (double)j / (double)img_out->height);
        fflush(stdout);
    }
    printf("\n");
}

struct PPMPixel get_gausian_3(struct ppm_array *img_in, int x, int y)
{
    double kernel[3][3] = {{0.01, 0.08, 0.01},
                           {0.08, 0.64, 0.08},
                           {0.01, 0.08, 0.01}};

    double red = 0;
    double green = 0;
    double blue = 0;
    for (int i = x - 1 < 0 ? 0 : -1; i <= 1 && x + i < img_in->width; i++)
    {
        for (int j = y - 1 < 0 ? 0 : -1; j <= 1 && y + j < img_in->height; j++)
        {
            red += kernel[i + 1][j + 1] * (double)img_in->arr[x + i][y + j]->red;
            blue += kernel[i + 1][j + 1] * (double)img_in->arr[x + i][y + j]->blue;
            green += kernel[i + 1][j + 1] * (double)img_in->arr[x + i][y + j]->green;
        }
    }
    struct PPMPixel pix;
    pix.red = (int)red;
    pix.green = (int)green;
    pix.blue = (int)blue;
    return pix;
}

void blur_gausian(struct ppm_array *img_in, struct ppm_array *img_out)
{
    for (int j = 0; j < img_out->height; j++)
    {
        for (int i = 0; i < img_out->width; i++)
        {
            struct PPMPixel pix = get_gausian_3(img_in, i, j);
            img_out->arr[i][j]->red = pix.red;
            img_out->arr[i][j]->green = pix.green;
            img_out->arr[i][j]->blue = pix.blue;
        }
    }
}

struct PPMPixel get_pix_avg(struct ppm_array *img_in, int x_1, int y_1, int x_2, int y_2)
{
    double red = 0;
    double green = 0;
    double blue = 0;
    int num_pix = 0;
    for (int i = x_1; i <= x_2 && i < img_in->width; i++)
    {
        for (int j = y_1; j <= y_2 && j < img_in->height; j++)
        {
            red += (double)img_in->arr[i][j]->red;
            green += (double)img_in->arr[i][j]->green;
            blue += (double)img_in->arr[i][j]->blue;
            num_pix++;
        }
    }
    red /= num_pix;
    green /= num_pix;
    blue /= num_pix;
    struct PPMPixel pix;
    pix.red = (int)red;
    pix.green = (int)green;
    pix.blue = (int)blue;
    return pix;
}

void resize_down_half(struct ppm_array *img_in, struct ppm_array *img_out)
{

    struct ppm_array blurred;
    blurred.height = img_in->height;
    blurred.width = img_in->width;
    ppm_array_allocate(&blurred);
    blur_gausian(img_in, &blurred);

    img_out->height = img_in->height / 2;
    img_out->width = img_in->width / 2;
    ppm_array_allocate(img_out);

    for (int j = 0; j < img_out->height; j++)
    {
        for (int i = 0; i < img_out->width; i++)
        {
            struct PPMPixel pix = get_pix_avg(&blurred, i * 2, j * 2, (i * 2) + 1, (j * 2) + 1);
            img_out->arr[i][j]->red = pix.red;
            img_out->arr[i][j]->green = pix.green;
            img_out->arr[i][j]->blue = pix.blue;
        }
    }
}

// TODO unfuck
void pyramid_block_match(struct ppm_array *img_left, struct ppm_array *img_right, struct disparity_map *disparity_map)
{
    struct ppm_array img_left_coarse;
    struct ppm_array img_right_coarse;
    struct disparity_map disparity_map_coarse;
    resize_down_half(img_left, &img_left_coarse);
    resize_down_half(img_right, &img_right_coarse);

    disparity_map_coarse.height = img_left->height;
    disparity_map_coarse.width = img_left->width;
    allocate_disparity_map(&disparity_map_coarse);

    block_match(&img_left_coarse, &img_right_coarse, &disparity_map_coarse, BLOCK_SIZE / 2);
    // memcpy(disparity_map, &disparity_map_coarse, sizeof(disparity_map_coarse));
    block_match_informed(img_left, img_right, &disparity_map_coarse, disparity_map, 2);
}

int main()
{
    // Load an image
    struct PPMImage *temp;
    struct ppm_array img_1;
    struct ppm_array img_2;
    struct disparity_map img_3;
    clock_t t;
    temp = readPPM("tsukuba/scene1.row3.col1.ppm");
    img_to_arr(temp, &img_1);
    free(temp->data);
    free(temp);
    temp = readPPM("tsukuba/scene1.row3.col2.ppm");
    img_to_arr(temp, &img_2);

    img_3.height = img_1.height;
    img_3.width = img_1.width;
    allocate_disparity_map(&img_3);

    // print_arr(img);

    // to_greyscale(&img_1);
    // to_greyscale(&img_2);

    t = clock();
    // resize_down_half(&img_1, &img_3);
    // blur_gausian(&img_1,&img_3);
    block_match(&img_1, &img_2, &img_3, 20);
    // pyramid_block_match(&img_1, &img_2, &img_3);
    t = clock() - t;
    printf("block_match() took %f seconds to execute \n", ((double)t) / CLOCKS_PER_SEC);

    // Export the processed image
    // arr_to_img(&img_3, temp);
    disparity_map_to_img(&img_3, temp);
    writePPM("processed.ppm", temp);
    free(temp->data);
    free(temp);
    free_ppm_array(&img_1);
    free_ppm_array(&img_2);
    free_disparity_map(&img_3);
}