// Nico Zucca, 12/2022

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <time.h>

#define KERNEL_EDGE_SIZE 5 // How many pixels above/below/left/right of given pixel to SAD for disparity metric
#define BLOCK_SIZE 50 // How many pixels right of given to check for disparity metric
#define RGB_COMPONENT_COLOR 255 // Used for exporting the ppm file

// Compile cmd:
// gcc -g main.c -o main


struct PPMPixel
{
    unsigned char red, green, blue;
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

// Frees space from the ppm_array.
void ppm_array_free(struct ppm_array *obj)
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

// Converts the array back into an image, for writing.
void arr_to_img(struct ppm_array *obj, struct PPMImage *img)
{
    int i;
    int x;
    int y;
    x = y = 0;
    for(i=0;i<img->x*img->y;i++)
    {
        img->data[i].red = obj->arr[x][y]->red;
        img->data[i].green = obj->arr[x][y]->green;
        img->data[i].blue = obj->arr[x][y]->blue;
        x++;
        if(x >= img->x)
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
    //open PPM file for reading
    fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Unable to open file '%s'\n", filename);
        exit(1);
    }

    //read image format
    if (!fgets(buff, sizeof(buff), fp)) {
        perror(filename);
        exit(1);
    }

    //check the image format
    if (buff[0] != 'P' || buff[1] != '6') {
         fprintf(stderr, "Invalid image format (must be 'P6')\n");
         exit(1);
    }

    //alloc memory form image
    img = (struct PPMImage *)malloc(sizeof(struct PPMImage));
    if (!img) {
         fprintf(stderr, "Unable to allocate memory\n");
         exit(1);
    }

    //check for comments
    c = getc(fp);
    while (c == '#') {
    while (getc(fp) != '\n') ;
         c = getc(fp);
    }

    ungetc(c, fp);
    //read image size information
    if (fscanf(fp, "%d %d", &img->x, &img->y) != 2) {
         fprintf(stderr, "Invalid image size (error loading '%s')\n", filename);
         exit(1);
    }

    //read rgb component
    if (fscanf(fp, "%d", &rgb_comp_color) != 1) {
         fprintf(stderr, "Invalid rgb component (error loading '%s')\n", filename);
         exit(1);
    }

    //check rgb component depth
    if (rgb_comp_color!= RGB_COMPONENT_COLOR) {
         fprintf(stderr, "'%s' does not have 8-bits components\n", filename);
         exit(1);
    }

    while (fgetc(fp) != '\n') ;
    //memory allocation for pixel data
    img->data = (struct PPMPixel*)malloc(img->x * img->y * sizeof(struct PPMPixel));

    if (!img) {
         fprintf(stderr, "Unable to allocate memory\n");
         exit(1);
    }

    //read pixel data from file
    if (fread(img->data, 3 * img->x, img->y, fp) != img->y) {
         fprintf(stderr, "Error loading image '%s'\n", filename);
         exit(1);
    }

    fclose(fp);
    return img;
}

// Prints the image to the cmd line, 1 pixel at a time. For debugging.
void print_img(struct PPMImage *img)
{
    printf("x,y : %d,%d\n",img->x,img->y);
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
    printf("width,height : %d,%d\n",obj.width,obj.height);
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
    //open file for output
    fp = fopen(filename, "wb");
    if (!fp) {
         fprintf(stderr, "Unable to open file '%s'\n", filename);
         exit(1);
    }

    //write the header file
    //image format
    fprintf(fp, "P6\n");

    //image size
    fprintf(fp, "%d %d\n",img->x,img->y);

    // rgb component depth
    fprintf(fp, "%d\n",RGB_COMPONENT_COLOR);

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
    if(in<0)
    {
        return -in;
    }
    return in;
}

int pixel_dif_abs(int x_1, int y_1, int x_2, int y_2, struct ppm_array *img_left, struct ppm_array *img_right)
{
    // Return -1 if out of bounds, so that we can process out of bounds differently
    if (x_1 < 0 || x_2 < 0 || y_1 < 0 || y_2 < 0 ||
        x_1 >= img_left->width || x_2 >= img_right->width || y_1 >= img_right->width || y_2 >= img_right->height)
    {
        // printf("OOB: x1 %d y1 %d x2 %d y2 %d 1c %d 1r %d 2c %d 2r %d\n",x_1,y_1,x_2,y_2,image_1.cols,image_1.rows,image_2.cols,image_2.rows);
        return -1;
    }
    int dif  =  0;
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
            for (int k = 0; k < 3; k++)
            {
                int diff = pixel_dif_abs(x_1 + i, y_1 + j, x_2 + i, y_2 + j, img_left, img_right);
                if (diff != -1)
                {
                    pixels++;
                    SAD += diff;
                }
            }
        }
    }

    // Divide by the number of valid pixels, to get average match
    SAD = SAD / (double)pixels;
    return SAD;
}

double disparity(struct ppm_array *img_left, struct ppm_array *img_right, int x, int y)
{
    double min_SAD = DBL_MAX;
    int disparity = 0;
    for(int i = 0; i <= BLOCK_SIZE && i + x + KERNEL_EDGE_SIZE> 0; i--)
    {
        // printf("i:%d x:%d KERNEL_EDGE_SIZE:%d img_left->width:%d\n",i,x,KERNEL_EDGE_SIZE,img_left->width);
        double new_SAD = get_sum_absolute_difference(x,y,x+i,y,img_left,img_right);
        if (new_SAD < min_SAD)
        {
            min_SAD = new_SAD;
            disparity = -i;
        }
        
    }
    return disparity;
}

double clamp_and_scale_256(double in_bottom, double in_top, double input)
{
    double out_top = 255;
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

void block_match(struct ppm_array *img_left, struct ppm_array *img_right, struct ppm_array *img_out)
{
    for (int j = 0; j < img_out->height; j++)
    {
        for (int i = 0; i < img_out->width; i++)
        {
            int newval = clamp_and_scale_256(0,50, disparity(img_left,img_right,i,j));
            img_out->arr[i][j]->red =
                img_out->arr[i][j]->green =
                    img_out->arr[i][j]->blue = newval;
        }
        printf("\r%d/%d - %2.0f%%",j,img_out->height, 100*(double)j/(double)img_out->height);
        fflush(stdout);
    }
    printf("\n");
}

int main()
{
    // Load an image
    struct PPMImage* temp;
    struct ppm_array img_1;
    struct ppm_array img_2;
    struct ppm_array img_3;
    temp = readPPM("tsukuba/scene1.row3.col1.ppm");
    img_to_arr(temp,&img_1);
    free(temp->data);
    free(temp);
    temp = readPPM("tsukuba/scene1.row3.col2.ppm");
    img_to_arr(temp,&img_2);

    img_3.height = img_1.height;
    img_3.width = img_1.width;
    ppm_array_allocate(&img_3);

    // print_arr(img);

    to_greyscale(&img_1);
    to_greyscale(&img_2);
    clock_t t;
    t = clock();
    block_match(&img_1, &img_2, &img_3);
    t = clock() - t;
    double time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds
    printf("block_match() took %f seconds to execute \n", time_taken);

    // Export the processed image
    arr_to_img(&img_3,temp);
    writePPM("processed.ppm", temp);
    free(temp->data);
    free(temp);
    ppm_array_free(&img_1);
    ppm_array_free(&img_2);
    ppm_array_free(&img_3);
}