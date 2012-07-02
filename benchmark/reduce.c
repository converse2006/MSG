#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#include "msg.h"

#ifndef DATA_SIZE
#define DATA_SIZE 4
#endif
#ifndef TIMES
#define TIMES 100
#endif

#define BILLION     1000000000L

int main(int argc,char *argv[]){

    printf("Program Start!\n");

    int i,j;
    int rank;
    struct timeval tv,tv2;
    unsigned long long start_time,end_time;

    MSG_ERR_T rc;

    MSG_Status status;
//    char buffer[DATA_SIZE];
    int sum;
    int result;


    rc = msg_init(argc,argv);
    if(rc != MSG_SUCCESS)
    {
        fprintf(stderr,"msg_init failed\n");
        exit(-1);
    }

    msg_comm_rank(MSG_COMM_WORLD,&rank);
    
    sum = rank;
    //printf("[%d] After initialized!\n",rank);

    if(rank == 0)
    {
        //memset(buffer,7,DATA_SIZE);
        start_time = msg_wtime();
    }

    for(i=0;i<TIMES;i++)
    {
        //printf("GOD\n");
        //if(rank != 0)
          //memset(buffer,'\0',DATA_SIZE);
        rc = msg_reduce((void *)&sum,(void *)&result,1,MSG_INT,MSG_SUM,0,MSG_COMM_WORLD);
    }

    if(!rank)
    {
        end_time = msg_wtime();

        printf("result: %d\n",result);

        fprintf(stderr,"DATA SIZE = %d\n",DATA_SIZE);
        fprintf(stderr,"TIMES = %d\n",TIMES);
        fprintf(stderr,"total time(s) = %lf\n",(double)(end_time-start_time)/(double)1000000);
        fprintf(stderr,"total time(us) = %llu\n",end_time-start_time);
        fprintf(stderr,"total time(ns) = %llu\n",(end_time-start_time) * 1000);
        fprintf(stderr,"average time(s) = %lf\n",((double)(end_time-start_time)/(double)TIMES)/(double)1000000);
        fprintf(stderr,"average time(us) = %llu\n",(end_time-start_time)/TIMES);
        fprintf(stderr,"average time(ns) = %llu\n",((end_time-start_time)*1000)/TIMES);
    }

/*
    sleep(rank);
    printf("[%d]#########################\n",rank);
    for(i=0;i<DATA_SIZE;i++)
        printf("%d ",buffer[i]);
    printf("\n");
    printf("[%d]#########################\n",rank);
*/ 

    msg_exit();
    return 0;
}
