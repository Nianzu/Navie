// gcc test.c -o test.o `sdl2-config --cflags --libs` -lm -O3

#include "icp.c"
#define PRECISION 0.01
int matrix_eq_2(double (*a)[4], double (*b)[4])
{
    if (fabs((*a)[0] - (*b)[0]) < PRECISION &&
        fabs((*a)[1] - (*b)[1]) < PRECISION &&
        fabs((*a)[2] - (*b)[2]) < PRECISION &&
        fabs((*a)[3] - (*b)[3]) < PRECISION)
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

int test_eigenvalues_2()
{
    printf("eigenvalues_2\n");
    if (1)
    {
        printf("\tTest 1234");
        double input[4] = {1, 2, 3, 4};
        double e[4] = {0, 0, 0, 0};

        double e_exp[4] = {5.37228, 0, 0, -0.372281};
        int success = eigenvalues_2(&input, &e);
        printf(matrix_eq_2(&e, &e_exp) ? " PASS" : " FAIL");
        printf("\n");
    }
    if (1)
    {
        printf("\tTest 1203");
        double input[4] = {1, 2, 0, 3};
        double e[4] = {0, 0, 0, 0};

        double e_exp[4] = {3, 0, 0, 1};
        int success = eigenvalues_2(&input, &e);
        printf(matrix_eq_2(&e, &e_exp) ? " PASS" : " FAIL");
        printf("\n");
    }
    if (1)
    {
        printf("\tTest 5492");
        double input[4] = {5,4,9,2};
        double e[4] = {0, 0, 0, 0};

        double e_exp[4] = {9.684658438426491, 0, 0, -2.684658438426491};
        int success = eigenvalues_2(&input, &e);
        printf(matrix_eq_2(&e, &e_exp) ? " PASS" : " FAIL");
        printf("\n");
    }
    if (1)
    {
        printf("\tTest complex");
        double input[4] = {1,-1,1,1};
        double e[4] = {0, 0, 0, 0};
        int success = eigenvalues_2(&input, &e);
        printf(!success ? " PASS" : " FAIL");
        printf("\n");
    }
    if (1)
    {
        printf("\tTest 5492");
        double input[4] = {2,2,5,-1};
        double e[4] = {0, 0, 0, 0};

        double e_exp[4] = {4, 0, 0, -3};
        int success = eigenvalues_2(&input, &e);
        printf(matrix_eq_2(&e, &e_exp) ? " PASS" : " FAIL");
        printf("\n");
    }
}

int test_eigenvectors_2()
{
    printf("eigenvectors_2\n");
    if (1)
    {
        printf("\tTest 1203");
        double input[4] = {1, 2, 0, 3};
        double sigma[4] = {3, 0, 0, 1};
        double ev[4] = {0, 0, 0, 0};

        double ev_exp[4] = {1, 1, 1, 0};
        eigenvectors_2(&input,&sigma, &ev);
        printf(matrix_eq_2(&ev, &ev_exp) ? " PASS" : " FAIL");
        printf("\n");
    }
    if (1)
    {
        printf("\tTest 225-1");
        double input[4] = {2,2,5,-1};
        double sigma[4] = {4, 0, 0, -3};
        double ev[4] = {0, 0, 0, 0};

        double ev_exp[4] = {1, 1, 1, -2.5};
        eigenvectors_2(&input,&sigma, &ev);
        printf(matrix_eq_2(&ev, &ev_exp) ? " PASS" : " FAIL");
        printf("\n");
    }
    if (1)
    {
        printf("\tTest 222-1");
        double input[4] = {2,2,2,-1};
        double sigma[4] = {3, 0, 0, -2};
        double ev[4] = {0, 0, 0, 0};

        double ev_exp[4] = {1, 1, 0.5, -2};
        eigenvectors_2(&input,&sigma, &ev);
        printf(matrix_eq_2(&ev, &ev_exp) ? " PASS" : " FAIL");
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
        double a[4] = {1, 2, 3, 4};
        double b = 0;
        double b_exp = -2;
        determinant_2(&a, &b);
        printf(b == b_exp ? " PASS" : " FAIL");
        printf("\n");
    }
}

int test_inverse_2()
{
    printf("inverse_2\n");
    if (1)
    {
        printf("\tTest zeros");
        double a[4] = {0, 0, 0, 0};
        double a_exp[4] = {0, 0, 0, 0};
        inverse_2(&a);
        printf(matrix_eq_2(&a, &a_exp) ? " PASS" : " FAIL");
        printf("\n");
    }
    if (1)
    {
        printf("\tTest 1234");
        double a[4] = {1, 2, 3, 4};
        double a_exp[4] = {-2,1,1.5,-0.5};
        inverse_2(&a);
        printf(matrix_eq_2(&a, &a_exp) ? " PASS" : " FAIL");
        printf("\n");
    }
}

int test_svd_2_new()
{
    printf("svd_2_new\n");
    if (1)
    {
        printf("\tTest 1234");
        double input[4] = {1, 2, 3, 4};
        double u[4] = {0, 0, 0, 0};
        double sigma[4] = {0, 0, 0, 0};
        double v[4] = {0, 0, 0, 0};

        double u_exp[4] = {0.4047, 0.9125, 0.9144, -0.4091};
        double sigma_exp[4] = {5.4653, 0, 0, 0.3606};
        double v_exp[4] = {0.5735, 0.8192, -0.8176, 0.5758};
        svd_2_new(&input, &u, &sigma, &v);
        printf(matrix_eq_2(&u, &u_exp) ? " PASS" : " FAIL");
        printf(matrix_eq_2(&sigma, &sigma_exp) ? " PASS" : " FAIL");
        printf(matrix_eq_2(&v, &v_exp) ? " PASS" : " FAIL");
        printf("\n");
    }
}

int main(int argc, char *argv[])
{
    test_transpose_2();
    test_cross_product_2();
    test_svd_2();
    test_determinant_2();
    test_eigenvalues_2();
    test_eigenvectors_2();
    test_inverse_2();
    test_svd_2_new();
}