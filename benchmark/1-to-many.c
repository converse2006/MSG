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
    char buffer[DATA_SIZE];


    rc = msg_init(argc,argv);
    if(rc != MSG_SUCCESS)
    {
        fprintf(stderr,"msg_init failed\n");
        exit(-1);
    }

    msg_comm_rank(MSG_COMM_WORLD,&rank);

    printf("[%d] After initialized!\n",rank);


    if(!rank)
    {
        //memset(buffer,'\0',DATA_SIZE);

        gettimeofday(&tv,NULL);
        start_time = tv.tv_sec * 1000000 + tv.tv_usec;

        for(i=1;i<48;i++)
        {
            memset(buffer,i,DATA_SIZE);

            msg_send((void *)buffer,DATA_SIZE,MSG_CHAR,i,DATA_SIZE,MSG_COMM_WORLD);

#ifdef ERROR_CHECK
            for(j=0;j<DATA_SIZE;j++)
                printf("%d ",buffer[j]);
            printf("\n");
#endif
        }

        gettimeofday(&tv2,NULL);
        end_time = tv2.tv_sec * 1000000 + tv2.tv_usec;

        fprintf(stderr,"Data Size = %d\n",DATA_SIZE);
        fprintf(stderr,"total time(s) = %lf\n",(double)(end_time-start_time)/(double)1000000);
        fprintf(stderr,"total time(us) = %llu\n",end_time-start_time);
        fprintf(stderr,"total time(ns) = %llu\n",(end_time-start_time) * 1000);
        fprintf(stderr,"average time(s) = %lf\n",((double)(end_time-start_time)/(double)TIMES)/(double)1000000);
        fprintf(stderr,"average time(us) = %llu\n",(end_time-start_time)/TIMES);
        fprintf(stderr,"average time(ns) = %llu\n",((end_time-start_time)*1000)/TIMES);
    }
    else{
            msg_recv((void *)buffer,DATA_SIZE,MSG_CHAR,0,DATA_SIZE,MSG_COMM_WORLD,&status);

#ifdef ERROR_CHECK
            for(j=0;j<DATA_SIZE;j++)
                buffer[j] += 1;
#endif

       //     msg_send((void *)buffer,DATA_SIZE,MSG_CHAR,0,DATA_SIZE,MSG_COMM_WORLD);


//            printf("[%d]#########################\n",rank);
            for(j=0;j<DATA_SIZE;j++)
                printf("%d ",buffer[j]);
            printf("\n");
//            printf("[%d]#########################\n",rank);

            //memset(buffer,'\0',DATA_SIZE);

     //   }

    //}
}

    //printf("Hello! I'm rank %d\n",rank);

    msg_exit();
    return 0;
}
