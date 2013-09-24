#include <stdio.h>
#include <mpi.h>

int main (int argc, char **argv)
{
  double sendTime, recvTime;
  int sz, myid, rc;
  int buf1 = 1, buf2 = 1;
  MPI_Status stat;
  MPI_Init (&argc, &argv);
  MPI_Comm_size (MPI_COMM_WORLD, &sz);
  MPI_Comm_rank (MPI_COMM_WORLD, &myid);
  int dest = ( myid + 1 ) % sz;//destination:  the neighbor process

  if(myid % 2){
    int dest = ( myid + 1 ) % sz;
    sendTime = MPI_Wtime();
    MPI_Send (&buf1, 1, MPI_INTEGER, dest, 1, MPI_COMM_WORLD);
    rc = MPI_Recv (&buf2, 1, MPI_INTEGER, dest, 1, MPI_COMM_WORLD, &stat);
    recvTime = MPI_Wtime();

    printf ("Send Time: %d, Recieved Time: %d, Time Elapsed: %d", sendTime, recvTime, recvTime-sendTime);
  }else{
    int source = (myid - 1) % sz;
    MPI_Recv (&buf2, 1, MPI_INTEGER, source, 1, MPI_COMM_WORLD, &stat);
    MPI_Send (&buf1, 1, MPI_INTEGER, source, 1, MPI_COMM_WORLD);
    printf ("Recieved from %d", source);
  }
  MPI_Finalize ();
  exit (0);
}

