/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

//#include "mpi.h"
#include "msg.h"
#include <stdio.h>
#include <string.h>

#define BUFLEN 512

int main(int argc, char *argv[])
{
    int i;
    int myid, numprocs, next, namelen;
    char buffer[BUFLEN];
    //MPI_Status status;
    MSG_Status status;

    //MPI_Init(&argc,&argv);
    //MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
    //MPI_Comm_rank(MPI_COMM_WORLD,&myid);
    msg_init(argc,argv);
    msg_comm_size(MSG_COMM_WORLD,&numprocs);
    msg_comm_rank(MSG_COMM_WORLD,&myid);
//    MPI_Get_processor_name(processor_name,&namelen);

  //  fprintf(stderr,"Process %d on %s\n", myid, processor_name);
    fprintf(stderr,"Process %d of %d\n", myid, numprocs);
    strcpy(buffer,"hello there");
    if (myid == numprocs-1)
	next = 0;
    else
	next = myid+1;

    if (myid == 0)
    {
        printf("%d sending '%s' \n",myid,buffer);fflush(stdout);
	//MPI_Send(buffer, strlen(buffer)+1, MPI_CHAR, next, 99, MPI_COMM_WORLD);
    for(i=1;i<numprocs;i++)
	    //msg_send(buffer, BUFLEN, MSG_CHAR, i, 99, MSG_COMM_WORLD);
	    msg_send(buffer, strlen(buffer)+1, MSG_CHAR, i, 99, MSG_COMM_WORLD);
	//printf("%d receiving \n",myid);fflush(stdout);
	//MPI_Recv(buffer, BUFLEN, MSG_CHAR, MPI_ANY_SOURCE, 99, MSG_COMM_WORLD,&status);
	//printf("%d received '%s' \n",myid,buffer);fflush(stdout);
	/* mpdprintf(001,"%d receiving \n",myid); */
    }
    else
    {
        printf("%d receiving  \n",myid);fflush(stdout);
	//MPI_Recv(buffer, BUFLEN, MPI_CHAR, 0, 99, MPI_COMM_WORLD,
	//	 &status);
	msg_recv(buffer, BUFLEN, MSG_CHAR, 0, 99, MSG_COMM_WORLD,&status);
	printf("%d received '%s' \n",myid,buffer);fflush(stdout);
	/* mpdprintf(001,"%d receiving \n",myid); */
	//MPI_Send(buffer, strlen(buffer)+1, MPI_CHAR, next, 99, MPI_COMM_WORLD);
	//printf("%d sent '%s' \n",myid,buffer);fflush(stdout);
    }
    msg_barrier(MSG_COMM_WORLD);
    msg_exit();
    //MPI_Barrier(MPI_COMM_WORLD);
    //MPI_Finalize();
    return (0);
}
