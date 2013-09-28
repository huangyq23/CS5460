#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <mw_api.h>
#include <string.h>
//test use case for a simple dot product work.
typedef struct work_t
{
    float vector1[10];
    float vector2[10];
} work_t;

typedef struct result_t
{
    float sub_result;
} result_t;

void generate_vector(int order, float vector[])
{
    int i;
    for (i = 0; i < order; i++)
    {
        vector[i] =  drand48();
    }
}

mw_works *create_work(int argc, char **argv, void *meta)
{
    int order = atoi(argv[1]);
    printf("order = %d\n", order);
    fflush(stdout);
    float *vector1 = (float *)malloc(sizeof(float) * order);
    float *vector2 = (float *)malloc(sizeof(float) * order);

    srand48(42); //Initialize a fixed seed for vector generation.
    generate_vector(order, vector1);
    generate_vector(order, vector2);

    int work_count = order / 10; //require order to be whole product of 10
    mw_works *works_list = malloc(sizeof(mw_works));
    works_list->size = work_count;

    work_t *works =  (work_t *)malloc(sizeof(work_t) * work_count);
    work_t *work = works;
    for (int i = 0; i < work_count; i++)
    {
        memcpy(work->vector1, vector1 + 10 * i, 10 * sizeof(float));
        memcpy(work->vector2, vector2 + 10 * i, 10 * sizeof(float));
        work++;
    }

    works_list->works = works;

    return works_list;
}

void *do_work(void *w)
{
    work_t *work = (work_t *) w;
    //print the received work
    printf("[");
    for (int i = 0; i < 10; i++)
    {
        printf(" %f ", work->vector1[i]);
    }
    printf("]\n");
    printf("[");
    for (int i = 0; i < 10; i++)
    {
        printf(" %f ", work->vector2[i]);
    }
    printf("]\n");

    result_t *result = (result_t *)malloc(sizeof(result_t));
    result->sub_result = 0;
    for (int i = 0; i < 10; i++)
    {
        result->sub_result += (work->vector1[i]) * (work->vector2[i]);
    }
    //print the local result
    printf("local result %f\n", result->sub_result);
    return (void *)result;
}

int process_results(int sz, void *res, void *meta)
{
    result_t *result = (result_t *)res;
    float fi_res = 0;
    for (int i = 0; i < sz; i++)
    {
        //print the received local result
        printf("received local result %f\n", result->sub_result);
        fi_res += result->sub_result;
        result++;
    }
    printf("The final result is %f\n", fi_res);
    return 1;
}


int main (int argc, char **argv)
{
    struct mw_api_spec f;
    int sz, myid;
    MPI_Init (&argc, &argv);

    f.create = create_work;
    f.result = process_results;
    f.compute = do_work;
    f.work_sz = sizeof (struct work_t);
    f.res_sz = sizeof (struct result_t);

    MW_Run (argc, argv, &f);

    return 0;

}
