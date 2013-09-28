#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <mw_api.h>
#include <string.h>
#define PATBUFSZ 64 //THe query string buffer size.
#define CHUNKSZ 4096*4 //The chunck size for worker data
//test use case for a simple dot product work.
typedef struct kmf_work_t
{
    char chunk[CHUNKSZ];
    char sep1;
    char pat[PATBUFSZ];
    char sep2;
} work_t;

typedef struct kmf_result_t
{
    long hits; //number of hits for the query string in a chuck
} result_t;

mw_works *create_work(int argc, char **argv, void *meta)
{
    char *f = argv[1];
    char *pat = argv[2];
    int patlen = strlen(argv[2]);
    if (patlen > PATBUFSZ - 1)
    {
        printf("Size of the query string must be smaller than the buffer size %d!\n", PATBUFSZ);
    }
    FILE *fp;
    long lSize;
    char *buffer;

    fp = fopen ( f , "rb" );
    if ( !fp ) perror(f), exit(1);

    fseek( fp , 0L , SEEK_END);
    lSize = ftell( fp );
    rewind( fp );

    /* allocate memory for entire content,padded with '\0' */
    buffer = (char *)calloc( 1, lSize + 1 );
    if ( !buffer ) fclose(fp), fputs("memory alloc fails", stderr), exit(1);

    /* copy the file into the buffer */
    if ( 1 != fread( buffer , lSize, 1 , fp) )
        fclose(fp), free(buffer), fputs("entire read fails", stderr), exit(1);
    printf("The file %s has been loaded into the RAM, the size of the file is %ld\n", f, lSize);
    //count the number of works
    int work_c = lSize / (CHUNKSZ - patlen + 1);
    int extra = lSize - work_c * (CHUNKSZ - patlen + 1);
    if (extra > 0) work_c++;
    printf("There are %d works", work_c);
    work_t *workArr = (work_t *)calloc(work_c, sizeof(work_t)); //separtor, pat and chunk are initialized with all '\0'
    mw_works *works = malloc(sizeof(mw_works));
    works->size = work_c;
    works->works = workArr;
    for (int i = 0; i < work_c; i++)
    {
        if (i < work_c - 1)
        {
            memcpy(workArr[i].chunk, buffer + i * (CHUNKSZ - patlen + 1), CHUNKSZ);
        }
        if (i == work_c - 1)
        {
            if (extra == 0)
            {
                memcpy(workArr[i].chunk, buffer + i * (CHUNKSZ - patlen + 1), CHUNKSZ);
            }
            else
            {
                memcpy(workArr[i].chunk, buffer + i * (CHUNKSZ - patlen + 1), extra);
            }
        }
        memcpy(workArr[i].pat, pat, patlen);
        //if(i<work_c )printf("The (%d) work:\n query:\n%s\n chun:\n%s\n",i, workArr[i].pat, workArr[i].chunk);
    }
    return works;
}

//function to prepare the finite automata for the query string
int *kmp_table(int length, char *W)
{
    int *T = (int *)malloc(sizeof(int) * length);
    int i = 2;
    int j = 0;
    T[0] = -1, T[1] = 0;
    while (i < length)
    {
        if (W[i - 1] == W[j])
        {
            T[i] = j + 1;
            ++j;
            ++i;
        }
        else if (j > 0)
        {
            j = T[j];
        }
        else
        {
            T[i] = 0;
            ++i;
            j = 0;
        }
    }
    //for(int index=0;index<length;index++){
    //printf("%d ",T[index]);
    //}
    //printf("\n");
    return T;

}

//return the number of non-overlapping matches of a quesry string W in the target string S
long kmp_search(int wl, int sl, char *W, char *S)
{
    int *T = kmp_table(wl, W);
    int m = 0;
    int i = 0;
    long match = 0;
    while (m + i < sl && S[m + i] != '\0')
    {
        if (S[m + i] == W[i])
        {
            ++i;
            //printf("%d\n",i);
        }
        else
        {
            m += i - T[i];
            if (i > 0) i = T[i];
        }
        if (i == wl)
        {
            match++;
            //printf("match at %d\n",m);
            m += wl;
            //fflush(stdout);
            i = 0;
        }
    }
    return match;
}

void *run_kmp(void *w)
{
    work_t *awork = (work_t *)w;
    char *query = awork->pat;
    char *target = awork-> chunk;
    //printf("recevied chunk:\n %s", target);
    //printf("received query:\n %s", query);
    result_t *result = (result_t *)malloc(sizeof(result_t));
    result->hits = kmp_search(strlen(query), CHUNKSZ, query, target);
    printf("local hits : %ld\n", result->hits);
    return (void *) result;
}

int process_results(int sz, void *res, void* meta)
{
    result_t *result = (result_t *)res;
    long fi_res = 0;
    for (int i = 0; i < sz; i++)
    {
        //print the received local result
        printf("received local result %ld\n", result->hits);
        fi_res += result->hits;
        result++;
    }
    printf("The total number of hits is %ld\n", fi_res);
    return 1;
}

int main (int argc, char **argv)
{
    struct mw_api_spec f;
    int sz, myid;
    MPI_Init (&argc, &argv);

    f.create = create_work;
    f.result = process_results;
    f.compute = run_kmp;
    f.work_sz = sizeof (work_t);
    f.res_sz = sizeof (result_t);
    f.meta_sz = 0;

    MW_Run (argc, argv, &f);

    return 0;

}

