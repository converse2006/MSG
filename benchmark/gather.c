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
    int numpros;
    struct timeval tv,tv2;
    unsigned long long start_time,end_time;
    unsigned int count=0;

    MSG_ERR_T rc;

    MSG_Status status;
//    char n[10],t[40];
    char *n,*t;

    rc = msg_init(argc,argv);
    if(rc != MSG_SUCCESS)
    {
        fprintf(stderr,"msg_init failed\n");
        exit(-1);
    }

    msg_comm_rank(MSG_COMM_WORLD,&rank);
    msg_comm_size(MSG_COMM_WORLD,&numpros);

    n= malloc(DATA_SIZE);
    t = malloc(DATA_SIZE*numpros);

    //printf("[%d] numpros:%d, After initialized!\n",rank,numpros);

    for(i=0;i<DATA_SIZE;i++)
        n[i] = rank;
    for(i=0;i<DATA_SIZE*numpros;i++)
        t[i] = -1; 

    if(!rank)
        start_time = msg_wtime();

    for(i=0;i<TIMES;i++)
    {
        //count++;
        //if(!rank)
            //memset(n,'\0',DATA_SIZE*numpros);
        rc = msg_gather(n,DATA_SIZE,MSG_CHAR,t,DATA_SIZE,MSG_CHAR,0,MSG_COMM_WORLD);
    }

    if(!rank)
    {
        end_time = msg_wtime();

        //printf("count:%d\n",count);
        fprintf(stderr,"DATA SIZE = %d\n",DATA_SIZE);
        fprintf(stderr,"TIMES = %d\n",TIMES);
        fprintf(stderr,"total time(s) = %lf\n",(double)(end_time-start_time)/(double)1000000);
        fprintf(stderr,"total time(us) = %llu\n",end_time-start_time);
        fprintf(stderr,"total time(ns) = %llu\n",(end_time-start_time) * 1000);
        fprintf(stderr,"average time(s) = %lf\n",((double)(end_time-start_time)/(double)TIMES)/(double)1000000);
        fprintf(stderr,"average time(us) = %llu\n",(end_time-start_time)/TIMES);
        fprintf(stderr,"average time(ns) = %llu\n",((end_time-start_time)*1000)/TIMES);
    }

    if(rank==0) 
        for(i=0;i<numpros*DATA_SIZE;i++){
            printf("%d,",t[i]);
            if((i%DATA_SIZE)==0&&i!=0)
                printf("\n");
        }   

    msg_exit();
    return 0;
}
