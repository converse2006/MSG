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

    //printf("Hello! I'm rank %d\n",rank);

    msg_exit();
    return 0;
}
