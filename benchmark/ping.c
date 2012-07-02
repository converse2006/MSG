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
    long count = 0;
    struct timeval tv,tv2;
    unsigned long long start_time,end_time;
    //unsigned long long time1,time2;

    MSG_ERR_T rc;

    MSG_Status status;
    char buffer[DATA_SIZE],check[DATA_SIZE];
    int  test[DATA_SIZE];


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
        //memset(buffer,7,DATA_SIZE);
        for(i=0;i<DATA_SIZE;i++)
            test[i] = i;
//        memset(test,7,DATA_SIZE*sizeof(int));
        memset(buffer,7,DATA_SIZE);
        memset(check,'\0',DATA_SIZE);

        gettimeofday(&tv,NULL);
        start_time = tv.tv_sec * 1000000 + tv.tv_usec;

        //time1 = msg_wtime();
        for(i=0;i<TIMES;i++)
        {
            //msg_send((void *)test,DATA_SIZE,MSG_INT,1,DATA_SIZE,MSG_COMM_WORLD);
            msg_send((void *)buffer,DATA_SIZE,MSG_CHAR,1,DATA_SIZE,MSG_COMM_WORLD);

//            msg_recv((void *)buffer,16384,MSG_CHAR,1,DATA_SIZE,MSG_COMM_WORLD,&status);
#ifdef ERROR_CHECK
            for(j=0;j<DATA_SIZE;j++)
            {
                check[j]+=1;
            }
            /*
               for(j=0;j<DATA_SIZE;j++)
               printf("%d ",buffer[j]);
               printf("\n");
             */
#endif
        }
        //time2 = msg_wtime();

        gettimeofday(&tv2,NULL);
        end_time = tv2.tv_sec * 1000000 + tv2.tv_usec;

        fprintf(stderr,"Data Size = %d\n",DATA_SIZE);
        fprintf(stderr,"total time(s) = %lf\n",(double)(end_time-start_time)/(double)1000000);
        fprintf(stderr,"total time(us) = %llu\n",end_time-start_time);
        fprintf(stderr,"total time(ns) = %llu\n",(end_time-start_time) * 1000);
        fprintf(stderr,"average time(s) = %lf\n",((double)(end_time-start_time)/(double)TIMES)/(double)1000000);
        fprintf(stderr,"average time(us) = %llu\n",(end_time-start_time)/TIMES);
        fprintf(stderr,"average time(ns) = %llu\n",((end_time-start_time)*1000)/TIMES);

        /*
        fprintf(stderr,"time2 = %llu\n",time2);
        fprintf(stderr,"time1 = %llu\n",time1);
        fprintf(stderr,"total = %llu\n",time2-time1);
        */

#ifdef ERROR_CHECK

        for(j=0;j<DATA_SIZE;j++)
        {
            if(buffer[j] != check[j])
                fprintf(stderr,"ERROR!\n");
            else
                count++;
        }
        if(count == DATA_SIZE)
            fprintf(stderr,"CORRECT!\n");
#endif
    }
    if(rank == 1)
    {
        for(i=0;i<TIMES;i++)
        {
            //msg_recv((void *)test,16384,MSG_INT,0,DATA_SIZE,MSG_COMM_WORLD,&status);
            msg_recv((void *)buffer,16384,MSG_CHAR,0,DATA_SIZE,MSG_COMM_WORLD,&status);

#ifdef ERROR_CHECK
            for(j=0;j<DATA_SIZE;j++)
                buffer[j] += 1;
#endif

//            msg_send((void *)buffer,DATA_SIZE,MSG_CHAR,0,DATA_SIZE,MSG_COMM_WORLD);

/*
            for(j=0;j<DATA_SIZE;j++)
                //printf("%d ",test[j]);
                printf("%d ",buffer[j]);
            printf("\n");

            memset(buffer,'\0',DATA_SIZE);
*/


        }

    }

    printf("Barrier\n");
    msg_barrier(MSG_COMM_WORLD);

    //printf("Hello! I'm rank %d\n",rank);

    msg_exit();
    return 0;
}
