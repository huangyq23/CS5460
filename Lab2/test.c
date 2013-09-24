#include<stdlib.h>
#include<mpi.h>
#include<mw_api.h>
#include<string.h>
#include<stdio.h>
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

void **create_work(int argc, char **argv)
{
    int order = atoi(argv[1]);
    printf("order = %d\n", order);
    fflush(stdout);
    float *vector1 = (float *)malloc(sizeof(float) * order);
    float *vector2 = (float *)malloc(sizeof(float) * order);
    for (int i = 0; i < order; i++)
    {
        vector1[i] = 1;
        vector2[i] = 1;
    }
    int work_c = order / 10; //require order to be whole product of 10
    work_t **works =  (work_t **)malloc(sizeof(void *) * (work_c + 1));
    for (int i = 0; i < work_c; i++)
    {
        work_t *w = (work_t *)malloc(sizeof(work_t));
        memcpy(w->vector1, vector1 + 10 * i, 10 * sizeof(float));
        memcpy(w->vector2, vector2 + 10 * i, 10 * sizeof(float));
        *(works + i) = w;
    }
    *(works + work_c) = NULL;
    return (void **)works;
}

char *do_work(char *work)
{
    work_t *awork = (work_t *) work;
    //print the received work
    printf("[");
    for (int i = 0; i < 10; i++)
    {
        printf(" %f ", awork->vector1[i]);
    }
    printf("]\n");
    printf("[");
    for (int i = 0; i < 10; i++)
    {
        printf(" %f ", awork->vector2[i]);
    }
    printf("]\n");

    result_t *result = (result_t *)malloc(sizeof(result_t));
    result->sub_result = 0;
    for (int i = 0; i < 10; i++)
    {
        result->sub_result += (awork->vector1[i]) * (awork->vector2[i]);
    }
    //print the local result
    printf("local result %f\n", result->sub_result);
    return (char *)result;
}

int process_results(int sz, char *res)
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

    MPI_Finalize ();

    return 0;

}
