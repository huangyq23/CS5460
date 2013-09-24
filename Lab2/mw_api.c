#include<stdlib.h>
#include<stdio.h>
#include<mw_api.h>
#include<mpi.h>
#include<string.h>

#define TAG_TERM 0
#define TAG_WORK 1
#define TAG_RESULT 1


void MW_Run (int argc, char **argv, struct mw_api_spec *f)
{
    int myid;
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

    if (myid == 0)
    {
        master(int argc, char **argv, f);
    }
    else
    {
        worker(int argc, char **argv, f);
    }


    MPI_Finalize();
    return 0;
}

static void master(int argc, char **argv, struct mw_api_spec *f)
{
    MPI_Status status;
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    mw_works work_list = f->create(argc, argv);
    void *work = work_list->works;

    void *results = malloc(res_sz * work_list->size);

    int rank = 0;
    //Send out new works
    for (rank = 1; rank < size; rank++)
    {
        MPI_Send(*work,
                 f->work_sz,
                 MPI_BYTE,
                 rank,
                 TAG_WORK,
                 MPI_COMM_WORLD);
        work = ((char*)work)+f->work_sz;
    }
    //If there are new works, recieve the result and assign new work
    while (work != NULL)
    {
        MPI_Recv(resp,
                 f->res_sz,
                 MPI_BYTE,
                 MPI_ANY_SOURCE,
                 TAG_RESULT,
                 MPI_COMM_WORLD,
                 &status);
        MPI_Send(*work,
                 f->work_sz,
                 MPI_BYTE,
                 rank,
                 TAG_WORK,
                 MPI_COMM_WORLD);
        work = ((char*)work)+f->work_sz;
    }
    //Recieve all works
    for (rank = 1; rank < size; rank++)
    {
        MPI_Recv(resp,
                 f->res_sz,
                 MPI_BYTE,
                 MPI_ANY_SOURCE,
                 TAG_RESULT,
                 MPI_COMM_WORLD,
                 &status);
    }
    //Terminate all workers
    for (rank = 1; rank < size; rank++)
    {
        MPI_Send(0,
                 0,
                 MPI_BYTE,
                 rank,
                 TAG_TERM,
                 MPI_COMM_WORLD);
    }

    f->result(work_list->size, results);
}

static void worker(int argc, char **argv, struct mw_api_spec *f)
{
    MPI_Status status;
    char *work = malloc(f->work_sz);
    while (1)
    {
        MPI_Recv(work,
                 f->work_sz,
                 MPI_BYTE,
                 0,
                 MPI_ANY_TAG,
                 MPI_COMM_WORLD,
                 &status);
        if (status.MPI_TAG == TAG_TERM)
        {
            break;
        }
        void *result = f->compute(work);
        MPI_Send(result,
                 f->res_sz,
                 MPI_BYTE,
                 0,
                 TAG_RESULT,
                 MPI_COMM_WORLD);
    }
    free(work);
}



