#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <mw_api_worker_master_failure_robust.h>
#include <mpi.h>
#include <string.h>

#define TAG_TERM 0
#define TAG_LIVE 1
//first 10 tags are reserved for future use
#define TAG_WORK_START 10
#define TAG_RESULT_START 10
typedef int bool;
#define true 1
#define false 0
static void master(int argc, char **argv, mw_api_spec *f, time_t timeout, float fp);
static void backup_master(int argc, char **argv, mw_api_spec *f, time_t timeout);
static void worker(int argc, char **argv, mw_api_spec *f, float fp, time_t timeout);
static int F_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, float fp);
static int F_Isend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,MPI_Request *request, float fp);
static int pick_dest(bool* failed, int size);
char* terminate_msg="All workers are probably failed based on your timeout, thus the computation is not recoverable!\nThe mpirun process should also be terminated. If the program hang on this message, then your timeout is probably too short, please terminate the program manually.\n";
void MW_Run (int argc, char **argv, mw_api_spec *f, float fp, time_t timeout)
{
	int myid;
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);

	if (myid == 0)
	master(argc, argv, f, timeout,fp);
	else if(myid == 1)
	backup_master(argc, argv, f, timeout);
	else
	worker(argc, argv, f,fp, timeout);
	MPI_Finalize();
}

static bool random_fail(float p)
{
	double rd = (double)rand() / (double)RAND_MAX;
	printf("Random number: %f\n", rd);
	if(rd <= (double)p){
	return true;
	}
	return false;
}

static bool master_live(time_t timeout, char* recv_result, int result_sz, MPI_Status* status){
	MPI_Request rrequest;
	bool flag;
	MPI_Irecv(recv_result,
	result_sz,
	MPI_BYTE,
	0,
	TAG_LIVE,
	MPI_COMM_WORLD,
	&rrequest);
	time_t recv_bg_t = time(NULL);
	while(time(NULL)<recv_bg_t + timeout){
		MPI_Test(&rrequest, &flag,status);
		if(flag){
			break;
		}
	}
		if(!flag){
			printf("master is likely failed, backup master now take over!\n");
		}
	return flag;
}

static int F_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, float fp)
{
	if (random_fail(fp)) {      
		printf("process failure!\n");
		MPI_Finalize();
		exit (0);
		return 0;
	} else {
		return MPI_Send (buf, count, datatype, dest, tag, comm);
	}
}

static int F_Isend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request* request, float fp)
{
	if (random_fail(fp)) {      
		printf("process failure!\n");
		MPI_Finalize();
		exit (0);
		return 0;
		
	} else{
		return MPI_Isend (buf, count, datatype, dest, tag, comm, request);
	}
}


//pick a live worker to the best of the master's knowledge
static int pick_dest(bool* failed, int size){
	int dest = 2;
	while(dest<size){
		if(failed[dest])    
		dest++;
		else
		break;
	}
	if(dest==size){
		printf("%s",terminate_msg);
		MPI_Finalize();
		exit(1);
	}
	return dest;
}  

static int pick_random_dest(bool* failed, int size){
	    int dest = 2;
    while(dest<size){
      if(failed[dest])    
         dest++;
      else
         break;
    }
    if(dest==size){
       printf("%s",terminate_msg);
       MPI_Finalize();
       MPI_Abort(MPI_COMM_WORLD,1);
       exit(1);
    }
    while(dest = random() % (size -2) + 2){
		printf("try %d\n", dest);
		if(!failed[dest]){
			printf("try succed %d\n", dest);
			return dest;
		}
	}
}

static void master(int argc, char **argv, mw_api_spec *f, time_t timeout, float fp)
{
	MPI_Status status;
	MPI_Request rrequest,srequest;
	int size;
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	void *meta = malloc(f->meta_sz);
	mw_works *work_list = f->create(argc, argv, meta);
	void *work = work_list->works;
	void *results = malloc(f->res_sz * work_list->size);
	void *recv_result = malloc(f->res_sz);
	bool *get_result = calloc(work_list->size, sizeof(bool));//whether we get the result for a specific work
	int *work_worker = calloc(work_list->size, sizeof(int)); //keep track of workers for each work
	bool *failed = calloc(size, sizeof(bool)); //an array of boolean, record whether a worker has failed
	void *result = results;

	int rank = 0;
	int resend = 0;
	int work_count = work_list->size;
	int work_finished = 0;
	int work_to_send = 0;
	int flag = 0;
	srand(time(NULL)*getpid());
	//Send out new works
	for (rank = 2; rank < size; rank++)
	{
		F_Send((char*)work + work_to_send*(f->work_sz),
		f->work_sz,
		MPI_BYTE,
		rank,
		TAG_WORK_START+work_to_send,
		MPI_COMM_WORLD,
		fp);

		work_worker[work_to_send] = rank;
		work_to_send++;
	}
	//If there are new works, recieve the result and assign new work
	while (work_finished < work_count)
	{
		MPI_Irecv(recv_result,
		f->res_sz,
		MPI_BYTE,
		MPI_ANY_SOURCE,
		MPI_ANY_TAG,
		MPI_COMM_WORLD,
		&rrequest);
		time_t recv_bg_t = time(NULL);
		while(time(NULL)<recv_bg_t + timeout){
			MPI_Test(&rrequest, &flag, &status);
			if(flag){
				
				int result_id = status.MPI_TAG-TAG_WORK_START;
				printf("received result %d from worker\n", result_id);
				memcpy(result+result_id*(f->res_sz), recv_result, f->res_sz);
				//send result to backup master
				F_Send(recv_result,
				f->res_sz,
				MPI_BYTE,
				1,
				status.MPI_TAG,
				MPI_COMM_WORLD,
				fp
				);
				get_result[result_id] = true;
				work_finished++;
				if(work_to_send < work_count){
				F_Send((char*)work + work_to_send*(f->work_sz),
				f->work_sz,
				MPI_BYTE,
				status.MPI_SOURCE,
				TAG_WORK_START+work_to_send,
				MPI_COMM_WORLD,
				fp);
				work_worker[work_to_send] = status.MPI_SOURCE;
				work_to_send++;
				}
				break;
		}
	}
	//if recieve time out, find out whether all workers are failed, if yes terminate, else send unfinished works to a live worker!
	if(!flag){
	MPI_Cancel(&rrequest);
	int fail = 0;
	for(int i=0;i<work_to_send;i++){
	if(!get_result[i]){
	fail++;
	failed[work_worker[i]]=true;
	}
	}
	resend++;
	if(fail>=size-2){// whether all workers are failed?
		printf("%s",terminate_msg);
		MPI_Finalize();
		exit(1);
	}
	for(int i=0;i<work_to_send;i++){
	if(!get_result[i]){//if a work is unfinished(master haven't got the result), pick a new worker.
	int dest =pick_random_dest(failed,size);
	F_Isend((char*)work + i*(f->work_sz),
	f->work_sz,
	MPI_BYTE,
	dest,
	TAG_WORK_START+i,
	MPI_COMM_WORLD,
	&srequest,
	fp);
	work_worker[i] = dest;
	}
	}
	}
	}

	//computation succeed, display how many workers have failed in the computation.
	printf("%d process have failed in this computation!\n",resend);
	f->result(work_count, results, meta);
	free(results);
	free(recv_result);
	free(get_result);
	free(work_worker);
	free(failed);
	MPI_Abort(MPI_COMM_WORLD,0);
}

static void backup_master(int argc, char **argv, mw_api_spec *f, time_t timeout)
{
	MPI_Status status;
	MPI_Request rrequest,srequest;
	int size;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	bool takeover = 0; //whether backup_master should take over
	void *meta = malloc(f->meta_sz);
	mw_works *work_list = f->create(argc, argv, meta);
	void *work = work_list->works;
	void *results = malloc(f->res_sz * work_list->size);
	void *recv_result = malloc(f->res_sz);
	bool *get_result = calloc(work_list->size, sizeof(bool));//whether we get the result for a specific work
	int *work_worker = calloc(work_list->size, sizeof(int)); //keep track of workers for each work
	bool *failed = calloc(size, sizeof(bool)); //an array of boolean, record whether a worker has failed
	void *result = results;

	int rank = 0;
	int resend = 0;
	int work_count = work_list->size;
	int work_finished = 0;
	int work_to_send = 0;
	int flag = 0;
	while(work_finished < work_count){
		if(master_live(2*timeout+1, recv_result, f->res_sz, &status)){
			int result_id = status.MPI_TAG - TAG_WORK_START;
			get_result[result_id] = 1;
			printf("backup master get the result %d\n", result_id);
			memcpy(result+result_id*(f->res_sz), recv_result, f->res_sz);
			work_finished++;
		}
		else{
			//master dead, takeover now
			takeover = 1;
			break;
		}
	}
		
    int backup_begin = 0;
	while (work_finished < work_count)
	{
		if(!backup_begin){
		    for(int i=0;i<work_count;i++){
               if(!get_result[i] ){//if a work is unfinished(master haven't got the result), pick a new worker.
                     int dest = pick_random_dest(failed,size);
                     MPI_Isend((char*)work + i*(f->work_sz),
	                 f->work_sz,
        	         MPI_BYTE,
                         dest,
                	 TAG_WORK_START+i,
                 	 MPI_COMM_WORLD,
                 	 &srequest);
                 	 printf("backup master send work %d to %d",i,dest);
                  work_worker[i] = dest;
               }
     	   }
     	   backup_begin++;
	   }
	   else{
     	    MPI_Irecv(recv_result,
                 f->res_sz,
                 MPI_BYTE,
                 MPI_ANY_SOURCE,
                 MPI_ANY_TAG,
                 MPI_COMM_WORLD,
                 &rrequest);
        time_t recv_bg_t = time(NULL);
        while(time(NULL)<recv_bg_t + timeout){
            MPI_Test(&rrequest, &flag, &status);
            if(flag){
                int result_id = status.MPI_TAG-TAG_WORK_START;
		memcpy(result+result_id*(f->res_sz), recv_result, f->res_sz);
                if(!get_result[result_id]){
		work_finished++;
		get_result[result_id]=true;
	}
                failed[status.MPI_SOURCE]=false;
				break;
			}
         }
       
       //if recieve time out, find out whether all workers are failed, if yes terminate, else send unfinished works to a live worker!
       if(!flag){
           MPI_Cancel(&rrequest);
           int fail = 0;
           for(int i=0;i<work_count;i++){
               if(!get_result[i]){
                  fail++;
                  failed[work_worker[i]]=true;
               }
			}
           if(fail==size-2){// whether all workers are failed?
               printf("%s",terminate_msg);
               MPI_Finalize();
               exit(1);
           }
            for(int i=0;i<work_count;i++){
               if(!get_result[i] ){//if a work is unfinished(master haven't got the result), pick a new worker.
                     int dest = pick_random_dest(failed,size);
                     MPI_Isend((char*)work + i*(f->work_sz),
	                 f->work_sz,
        	         MPI_BYTE,
                         dest,
                	 TAG_WORK_START+i,
                 	 MPI_COMM_WORLD,
                 	 &srequest);
                 	 printf("backup master send work %d to %d",i,dest);
                  work_worker[i] = dest;
               }
     	   }
           
	   }
   }
   }
   
     	   

	//computation succeed, display how many workers have failed in the computation.
	if(takeover){
		printf("master has failed in this computation!\n",resend);
		f->result(work_count, results, meta);
		free(results);
		free(recv_result);
		free(get_result);
		free(work_worker);
		free(failed);
		MPI_Abort(MPI_COMM_WORLD,1);
	}
}

static void worker(int argc, char **argv, mw_api_spec *f, float failure_p, time_t timeout)
{
	MPI_Status rstatus, sstatus;
	MPI_Request srequest;
	char *work = malloc(f->work_sz); //work received from master
	srand(time(NULL)*getpid());
	int flag = 0;
	while (1)
	{
		MPI_Recv(work,
		f->work_sz,
		MPI_BYTE,
		MPI_ANY_SOURCE,
		MPI_ANY_TAG,
		MPI_COMM_WORLD,
		&rstatus);
		
		void *result = f->compute(work);
		
		printf("worker begin to send result using tag %d\n", rstatus.MPI_TAG);
		 time_t result_end_t = time(NULL);
		F_Isend(result,
		f->res_sz,
		MPI_BYTE,
		rstatus.MPI_SOURCE,
		rstatus.MPI_TAG,
		MPI_COMM_WORLD,
		&srequest,
		failure_p);
		printf("F_isend fired for tag %d\n", rstatus.MPI_TAG);
		int i;
        while(time(NULL)<result_end_t + timeout){
			printf("inside loop\n");
            MPI_Test(&srequest, &flag, &sstatus);
            printf("%d mpi_test for receive",i++);
            if(flag){
				break;
				printf("worker has sent the result with tag %d\n",rstatus.MPI_TAG);
			}
		}
		if(!flag){
			MPI_Cancel(&srequest);
			printf("worker's result is not received\n");
		}
	}
	free(work);
}



