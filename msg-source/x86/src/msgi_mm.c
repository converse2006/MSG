/*
 * Copyright (C) 2011 PASLab CSIE NTU. All rights reserved.
 *      - Chiu Po-Hsun <maxshow0906@gmail.com>
 */

#include "msgi_mm.h"
#include "msg_internal.h"
#include "msg.h"

extern int local_rank;
extern int numpros;
extern int shmid;
extern char *shm_address;
extern long shm_address_location;
extern int non_block_shmid;
extern char *non_block_shm_address;
extern long non_block_shm_address_location;
#ifdef SERVICE_PROCESSOR
extern int service_processor_shmid;
extern char *service_processor_shm_address;
extern long service_processor_shm_address_location;
#endif

extern volatile char *barrier_address;
extern int local_barrier_flag;

MSG_ERR_T msgi_mm_init(void *argc,void *argv)
{
#ifdef SERVICE_PROCESSOR
    key_t key = x86_SERVICE_PROCESSOR_SHM_KEY;

    // get the shared memory id
    if((service_processor_shmid = shmget(key,SERVICE_PROCESSOR_SHM_SIZE,IPC_CREAT | 0666)) < 0)
    {perror("shmget failed");   exit(-1);}

    // Attach the segment to data space
    if((service_processor_shm_address = shmat(service_processor_shmid,NULL,0)) == (char *) -1)
    {perror("shmat failed");    exit(-1);}

    service_processor_shm_address_location = (long)service_processor_shm_address;
    //printf("service_processor_shm_address_location: %ld\n",service_processor_shm_address_location);
#else
    key_t key = x86_SHM_KEY;

    // get the shared memory id
    if((shmid = shmget(key,SHM_SIZE,IPC_CREAT | 0666)) < 0)
    {perror("shmget failed");   exit(-1);}

    // Attach the segment to data space
    if((shm_address = shmat(shmid,NULL,0)) == (char *) -1)
    {perror("shmat failed");    exit(-1);}

    shm_address_location = (long)shm_address;
/*
    key = x86_NON_BLOCK_SHM_KEY;

    // get the shared memory id
    if((non_block_shmid = shmget(key,NON_BLOCK_SHM_SIZE,IPC_CREAT | 0666)) < 0)
    {perror("shmget failed");   exit(-1);}

    // Attach the segment to data space
    if((non_block_shm_address = shmat(non_block_shmid,NULL,0)) == (char *) -1)
    {perror("shmat failed");    exit(-1);}

    non_block_shm_address_location = (long)non_block_shm_address;
*/
#endif
    return MSG_SUCCESS;
}

MSG_ERR_T msgi_mm_exit(void *argc,void *argv)
{
    int errno;
    int i;
    int child_status;
    struct shmid_ds *shmid_ds=NULL;

#ifdef SERVICE_PROCESSOR
    // Detach the shared memory
    if((errno = shmdt((void *)service_processor_shm_address)) == -1)
    {perror("shmdt failed");    exit(-1);}

    if(!local_rank)
    {
        for(i=0;i<numpros-1;i++)
        {
            //printf("I'm %d, now wait\n",local_rank);
            wait(&child_status);
        }

        //Need to MODIFY, only the main process can remove the segment
        // Remove the shared memory
        if((errno = shmctl(service_processor_shmid,IPC_RMID,shmid_ds)) == -1)
        {perror("shmctl failed");    exit(-1);}
        printf("Remove the shared region\n");
    }
    else
    {
        //printf("I'm %d, now exit\n",local_rank);
        exit(-1);
    }
#else
    // Detach the shared memory
    if((errno = shmdt((void *)shm_address)) == -1)
    {perror("shmdt failed");    exit(-1);}

    if(!local_rank)
    {
        for(i=0;i<numpros-1;i++)
        {
            //printf("I'm %d, now wait\n",local_rank);
            wait(&child_status);
        }

        //Need to MODIFY, only the main process can remove the segment
        // Remove the shared memory
        if((errno = shmctl(shmid,IPC_RMID,shmid_ds)) == -1)
        {perror("shmctl failed");    exit(-1);}
        printf("Remove the shared region\n");
    }
    else
    {
        //printf("I'm %d, now exit\n",local_rank);
        exit(-1);
    }
#endif
    return MSG_SUCCESS;
}
