// gcc test.c -o test.o `sdl2-config --cflags --libs` -lm -O3

#include "icp.c"
#define PRECISION 0.01
int matrix_eq_2(double (*a)[4], double (*b)[4])
{
    if ((*a)[0] - (*b)[0] < PRECISION &&
        (*a)[1] - (*b)[1] < PRECISION &&
        (*a)[2] - (*b)[2] < PRECISION &&
        (*a)[3] - (*b)[3] < PRECISION)
    {
        return 1;
    }
    printf(" E: %3f %3f %3f %3f A: %3f %3f %3f %3f", (*b)[0], (*b)[1], (*b)[2], (*b)[3], (*a)[0], (*a)[1], (*a)[2], (*a)[3]);
    return 0;
}

int test_transpose_2()
{
    printf("transpose_2\n");
    if (1)
    {
        printf("\tTest zeros");
        double a[4] = {0, 0, 0, 0};
        double a_exp[4] = {0, 0, 0, 0};
        transpose_2(&a);
        printf(matrix_eq_2(&a, &a_exp) ? " PASS" : " FAIL");
        printf("\n");
    }

    if (1)
    {
        printf("\tTest 1234");
        double a[4] = {1, 2, 3, 4};
        double a_exp[4] = {1, 3, 2, 4};
        transpose_2(&a);
        printf(matrix_eq_2(&a, &a_exp) ? " PASS" : " FAIL");
        printf("\n");
    }
}

int test_cross_product_2()
{
    printf("cross_product_2\n");
    if (1)
    {
        printf("\tTest zeros");
        double a[4] = {0, 0, 0, 0};
        double b[4] = {0, 0, 0, 0};
        double a_exp[4] = {0, 0, 0, 0};
        cross_product_2(&a, &b);
        printf(matrix_eq_2(&a, &a_exp) ? " PASS" : " FAIL");
        printf("\n");
    }

    if (1)
    {
        printf("\tTest 1234");
        double a[4] = {1, 2, 3, 4};
        double b[4] = {1, 2, 3, 4};
        double a_exp[4] = {7, 10, 15, 22};
        cross_product_2(&a, &b);
        printf(matrix_eq_2(&a, &a_exp) ? " PASS" : " FAIL");
        printf("\n");
    }

    if (1)
    {
        printf("\tTest 6085");
        double a[4] = {6, 0, 8, 5};
        double b[4] = {6, 0, 8, 5};
        double a_exp[4] = {36, 0, 88, 25};
        cross_product_2(&a, &b);
        printf(matrix_eq_2(&a, &a_exp) ? " PASS" : " FAIL");
        printf("\n");
    }
}

int test_svd_2()
{
    printf("svd_2\n");
    if (1)
    {
        printf("\tTest 1234");
        double input[4] = {1, 2, 3, 4};
        double u[4] = {0, 0, 0, 0};
        double sigma[4] = {0, 0, 0, 0};
        double v[4] = {0, 0, 0, 0};

        double u_exp[4] = {0.4047, -0.9125, 0.9144, 0.4091};
        double sigma_exp[4] = {5.4653, 0, 0, 0.3606};
        double v_exp[4] = {0.5735, 0.8192, 0.8176, -0.5758};
        svd_2(&input, &u, &sigma, &v);
        printf(matrix_eq_2(&u, &u_exp) ? " PASS" : " FAIL");
        printf(matrix_eq_2(&sigma, &sigma_exp) ? " PASS" : " FAIL");
        printf(matrix_eq_2(&v, &v_exp) ? " PASS" : " FAIL");
        printf("\n");
    }
}

int test_determinant_2()
{
    printf("determinant_2\n");
    if (1)
    {
        printf("\tTest zeros");
        double a[4] = {0, 0, 0, 0};
        double b = 0;
        double b_exp = 0;
        determinant_2(&a, &b);
        printf(b == b_exp ? " PASS" : " FAIL");
        printf("\n");
    }
    if (1)
    {
        printf("\tTest 1234");
        double a[4] = {1,2,3,4};
        double b = 0;
        double b_exp = -2;
        determinant_2(&a, &b);
        printf(b == b_exp ? " PASS" : " FAIL");
        printf("\n");
    }
}

int main(int argc, char *argv[])
{
    test_transpose_2();
    test_cross_product_2();
    test_svd_2();
    test_determinant_2();
}