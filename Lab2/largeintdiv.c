#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <mw_api.h>
#include <string.h>
#include <gmp.h>

typedef struct work_t
{
    unsigned long range_size;
    char target[256];
    char divisor[256];
} work_t;

typedef struct result_t
{
    int offsets_num;
    char divisor[256];
    int offsets[256];
} result_t;

typedef struct meta_t
{
    mpz_t target;
    double start_time;
} meta_t;

mw_works *create_work(int argc, char **argv, void *metaPtr)
{

    meta_t *meta = (meta_t *)metaPtr;

    mpz_t target, range, target_root, divisor, work_count_long;
    mpz_init_set_str (target, argv[1], 10);
    mpz_init_set_str (range, argv[2], 10);
    mpz_init_set_ui(divisor, 1);
    mpz_init(target_root);
    mpz_init(work_count_long);

    mpz_init_set(meta->target, target);

    //gmp_printf ("%Zd\n",  target);
    //gmp_printf ("%Zd\n",  range);

    mpz_sqrt(target_root, target);

    //mpz_get_str(meta->target, 10, target);

    //gmp_printf ("%Zd\n\n",  target_root);

    mpz_cdiv_q(work_count_long, target_root, range);

    int work_count = mpz_get_ui(work_count_long);
    unsigned long range_size = mpz_get_ui(range);

    mw_works *works_list = malloc(sizeof(mw_works));
    works_list->size = work_count;

    work_t *works =  (work_t *)malloc(sizeof(work_t) * work_count);
    work_t *work = works;
    for (int i = 0; i < work_count; i++)
    {
        work->range_size = range_size;

        //mpz_export (work->target, 256, 1, sizeof(char), 0, 0, target);
        //mpz_export (work->divisor, 256, 1, sizeof(char), 0, 0, divisor);

        mpz_get_str(work->target, 10, target);
        mpz_get_str(work->divisor, 10, divisor);

        mpz_add_ui (divisor, divisor, range_size);
        work++;
    }

    works_list->works = works;
    meta->start_time = MPI_Wtime();
    return works_list;
}

void *do_work(void *w)
{
    work_t *work = (work_t *) w;

    mpz_t divisor, target;
    //mpz_init(divisor);
    //mpz_init(target);

    result_t *result = (result_t *)malloc(sizeof(result_t));
    result->offsets_num = 0;
    memcpy(result->divisor, work->divisor, 256 * sizeof(char));
    // mpz_import (divisor, 256, 1, sizeof(char), 0, 0, work->divisor);
    // mpz_import (target, 256, 1, sizeof(char), 0, 0, work->target);
    mpz_init_set_str (divisor, work->divisor, 10);
    mpz_init_set_str (target, work->target, 10);

    //gmp_printf ("C %Zd, %Zd\n\n", target, divisor);

    for (int i = 0; i < work->range_size; i++)
    {
        if (mpz_divisible_p(target, divisor))
        {
            result->offsets[result->offsets_num++] = i;
        }
        mpz_add_ui (divisor, divisor, 1);
    }

    return (void *)result;
}

int process_results(int sz, void *res, void *metaPtr)
{
    mpz_t divisor;
    mpz_t factor;
    mpz_t factor2;
    mpz_init(factor);
    mpz_init(factor2);

    result_t *result = (result_t *)res;
    meta_t *meta = (meta_t *)metaPtr;
    //mpz_import (divisor, 256, 1, sizeof(char), 0, 0, result->divisor);

    //gmp_printf ("%s\n",  result->divisor);
    //printf ("xx: %s\n",  result->divisor);
    double elapsed_time = MPI_Wtime() - meta->start_time;


    printf("%.10f", elapsed_time);

    // for (int i = 0; i < sz; i++)
    // {
    //     mpz_init_set_str (divisor, result->divisor, 10);
    //     for (int j = 0; j < result->offsets_num; j++)
    //     {
    //         mpz_add_ui (factor, divisor, result->offsets[j]);
    //         mpz_cdiv_q(factor2, meta->target, factor);
    //         //gmp_printf ("%Zd\n",  meta->target);
    //         //gmp_printf ("%Zd\n",  factor);
    //         //gmp_printf ("%Zd\n",  factor2);
    //     }
    //     result++;
    // }
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
    f.work_sz = sizeof (work_t);
    f.res_sz = sizeof (result_t);
    f.meta_sz = sizeof (meta_t);

    MW_Run (argc, argv, &f);

    return 0;

}
