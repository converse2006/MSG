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


    rc = msg_init(argc,argv);
    if(rc != MSG_SUCCESS)
    {
        fprintf(stderr,"msg_init failed\n");
        exit(-1);
    }

    rc = msg_comm_rank(MSG_COMM_WORLD,&rank);

    if(!rank)
        start_time = msg_wtime();

    for(i=0;i<TIMES;i++)
        rc = msg_barrier(MSG_COMM_WORLD);

    if(!rank)
    {
        end_time = msg_wtime();

        //fprintf(stderr,"time2 = %llu\n",start_time);
        //fprintf(stderr,"time1 = %llu\n",end_time);
        fprintf(stderr,"TIMES = %d\n",TIMES);
        fprintf(stderr,"total time(s) = %lf\n",(double)(end_time-start_time)/(double)1000000);
        fprintf(stderr,"total time(us) = %llu\n",end_time-start_time);
        fprintf(stderr,"total time(ns) = %llu\n",(end_time-start_time) * 1000);
        fprintf(stderr,"average time(s) = %lf\n",((double)(end_time-start_time)/(double)TIMES)/(double)1000000);
        fprintf(stderr,"average time(us) = %llu\n",(end_time-start_time)/TIMES);
        fprintf(stderr,"average time(ns) = %llu\n",((end_time-start_time)*1000)/TIMES);
    }

    msg_exit();
    return 0;
}
