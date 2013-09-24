#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>


void generate_vector(int order, double vector[])
{
    int i;
    for (i = 0; i < order; i++)
    {
        vector[i] =  drand48();
    }
}

void print_vector(int order, double vector[])
{
    int i;
    for (i = 0; i < order; i++)
    {
        printf("%.5f ", vector[i]);
    }
    printf("\n\n");
}

int main (int argc, char **argv)
{

    MPI_Status stat;
    MPI_Init (&argc, &argv);
    int sz, myid;
    MPI_Comm_size (MPI_COMM_WORLD, &sz);
    MPI_Comm_rank (MPI_COMM_WORLD, &myid);
    double start_time;
    double end_time;
    double elasped_time;

    int order = -1, dest = -1;
    if (myid == 0)
    {
        printf("Enter the order of the vector: ");

        fflush(stdout);
        //abort();
        scanf("%d", &order);

        for (dest = 1; dest < sz; dest++)
        {
            MPI_Send (&order, 1, MPI_INTEGER, dest, 1, MPI_COMM_WORLD);
        }
    }
    else
    {
        MPI_Recv (&order, 1, MPI_INTEGER, 0, 1, MPI_COMM_WORLD, &stat);
    }
    int my_order = order / sz;

    double *vector1;
    double *vector2;
    double *vectorx;
    double *vectory;

    if (myid == 0)
    {
        vectorx = (double *)malloc(order * sizeof(double));
        vectory = (double *)malloc(order * sizeof(double));

        srand48(42); //Initialize a fixed seed for vector generation.
        generate_vector(order, vectorx);
        generate_vector(order, vectory);

        vector1 = vectorx;
        vector2 = vectory;


        start_time = MPI_Wtime();

        double *pointer_x = vectorx + my_order;
        double *pointer_y = vectory + my_order;
        for (dest = 1; dest < sz; dest++)
        {
            MPI_Send (pointer_x, my_order, MPI_DOUBLE, dest, 1, MPI_COMM_WORLD);
            MPI_Send (pointer_y, my_order, MPI_DOUBLE, dest, 2, MPI_COMM_WORLD);
            pointer_x+=my_order;
            pointer_y+=my_order;
        }
    }
    else
    {
        vector1 = (double *)malloc(my_order * sizeof(double));
        vector2 = (double *)malloc(my_order * sizeof(double));
        MPI_Recv (vector1, my_order, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, &stat);
        MPI_Recv (vector2, my_order, MPI_DOUBLE, 0, 2, MPI_COMM_WORLD, &stat);
    }

    double dot_product = 0;

    printf("%d %.10f %.10f\n\n", myid, vector1[0], vector2[0]);

    int k;
    for (k = 0; k < my_order; k++)
    {
        dot_product += vector1[k] * vector2[k];
    }

    if (myid == 0)
    {
        double temp_dot_product;
        for (dest = 1; dest < sz; dest++)
        {
            MPI_Recv (&temp_dot_product, my_order, MPI_DOUBLE, dest, 3, MPI_COMM_WORLD, &stat);
            dot_product += temp_dot_product;
        }
        end_time = MPI_Wtime();
        elasped_time = end_time - start_time;
        printf("%.10f\t%.10f\t%.10f\t%.10f\n", dot_product, start_time, end_time, elasped_time);
        free(vectorx);
        free(vectory);
    }
    else
    {
        MPI_Send (&dot_product, 1, MPI_DOUBLE, 0, 3, MPI_COMM_WORLD);
        free(vector1);
        free(vector2);
    }


    MPI_Finalize ();
    return 0;
}
