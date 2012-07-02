/* 
 * Copyright (C) 2011 PASLab CSIE NTU. All rights reserved.
 *      - Chiu Po-Hsun <maxshow0906@gmail.com>
 */

#include "msg.h"
#include "msg_internal.h"
#include "msgi_sbp.h"
#include "msgi_q.h"


// ---- Locals ----
long msgi_sbp_meta[ENV_CORE_NUM][MSGI_MAX_SBP_BUFFERS];
long msgi_sbp[ENV_CORE_NUM][MSGI_MAX_SBP_BUFFERS];


extern int local_rank;
extern long shm_address_location;
#ifdef SERVICE_PROCESSOR
extern long service_processor_shm_address_location;
#endif

/* RQ and SBP init--------------*/
//RQ meta-data
extern int msgi_rq_meta_size;
extern long msgi_rq_meta_start;
//RQ Data Buffer
extern int msgi_rq_buffer_size;
extern long msgi_rq_buffer_start;

#ifdef SERVICE_PROCESSOR
extern long msgi_rfq_meta_start;
extern long msgi_rfq_buffer_start;
#endif

extern int buffer_size;
extern int block_size;
//Send Buffer Pool meta-data
extern int msgi_sbp_meta_size;
extern long msgi_sbp_meta_start;
//Send Buffer Pool Data Buffer
extern int msgi_sbp_pool_size;
extern long msgi_sbp_buffer_start;
/*-------------------------------*/

// ---- Local prototypes ----
static inline unsigned int msgi_count_blocks(unsigned int size);
/*-------------------------------*/

MSG_ERR_T msgi_copy_to_sbp(int buf_idx, void *data,int sizeb)
{
    volatile msgi_sbp_meta_t *local_meta_ptr = NULL;
    volatile long local_buffer;
    unsigned char *p;
    int bcount;
    int index;
    int i;
    int level = 3;

    MSG_DBG(level, MESSAGE, "In msgmi_copy_to_sbp()");

    local_meta_ptr = (msgi_sbp_meta_t *) (uintptr_t)msgi_sbp_meta[local_rank][buf_idx];
    local_buffer = msgi_sbp[local_rank][buf_idx];

    MSG_DBG(level, MESSAGE, "(before)producer = %d", local_meta_ptr->producer);

    if(sizeb > block_size)   // now block size = 16KB
    {
        p = (unsigned char *)data;

        //block count
        bcount = msgi_count_blocks(sizeb);
        MSG_DBG(level, MESSAGE, "bcount = %d", bcount);

        //example:  data_size = 120K, block_size = 16K, buffer_size = 64K
        //          bcount = 8, for entry[0-6] is 16K and entry[7] is 120-(8-1)*16K = 8K
        //The for loop is handling the entry[0-6]
        for(i=0;i<(bcount-1);i++)
        {
            index = (local_meta_ptr->producer + 1) % MSGI_MAX_SBP_BLOCKS_PER_BUFFER;

            //when the producer is faster than consumer
            while(index == local_meta_ptr->consumer){}

            MSG_DBG(level, MESSAGE, "addr = %ld", (long)GET_ADDR(local_buffer, local_meta_ptr->producer, block_size));
            memcpy(GET_ADDR(local_buffer,local_meta_ptr->producer,block_size), p, block_size);

            local_meta_ptr->producer = index;

            p += block_size;
        }

        //handle the remain data, e.g. entry[7]
        index = (local_meta_ptr->producer + 1) % MSGI_MAX_SBP_BLOCKS_PER_BUFFER;

        while(index == local_meta_ptr->consumer){}

        MSG_DBG(level, MESSAGE, "addr = %ld", (long)GET_ADDR(local_buffer, local_meta_ptr->producer, block_size));
        memcpy(GET_ADDR(local_buffer,local_meta_ptr->producer,block_size), p,sizeb - (bcount - 1)* block_size);

        local_meta_ptr->producer = index;

        //mark the entry to BUSY
        local_meta_ptr->control_flag = BUSY;
    }
    else
    {
        index = (local_meta_ptr->producer + 1) % MSGI_MAX_SBP_BLOCKS_PER_BUFFER;

        //block is full
        while(index == local_meta_ptr->consumer){}

        MSG_DBG(level, MESSAGE, "addr = %ld", (long)GET_ADDR(local_buffer, local_meta_ptr->producer, block_size));
        memcpy(GET_ADDR(local_buffer,local_meta_ptr->producer,block_size),data,sizeb);

        local_meta_ptr->producer = index;

        //mark the entry to BUSY
        local_meta_ptr->control_flag = BUSY;
    }
    MSG_DBG(level, MESSAGE, "(after)producer = %d", local_meta_ptr->producer);

    MSG_DBG(level, MESSAGE, "Leaving msgmi_copy_to_shm() ...");

    return MSG_SUCCESS;
}

MSG_ERR_T msgi_copy_from_sbp(int src_rank,int buf_idx, void *data,int sizeb)
{
    volatile msgi_sbp_meta_t *remote_meta_ptr = NULL;
    volatile long remote_buffer;
    volatile unsigned char *p;
    volatile int index;
    int bcount;
    int i;
    int level = 3;

    MSG_DBG(level, MESSAGE, "In msgmi_copy_from_sbp()");

    remote_meta_ptr = (msgi_sbp_meta_t *)(uintptr_t)msgi_sbp_meta[src_rank][buf_idx];
    remote_buffer = msgi_sbp[src_rank][buf_idx];

    MSG_DBG(level, MESSAGE, "(before)consumer = %d", remote_meta_ptr->consumer);

    if(sizeb > block_size)   // now block size = 16KB
    {
        p = (unsigned char *)data;

        bcount = msgi_count_blocks(sizeb);
        MSG_DBG(level, MESSAGE, "bcount = %d", bcount);

        for(i=0;i<(bcount-1);i++)
        {
            // Check if the shared memory queue is not empty
            while(remote_meta_ptr->consumer == remote_meta_ptr->producer){}

            memcpy((void *)p, GET_ADDR(remote_buffer,remote_meta_ptr->consumer,block_size), block_size);

            index = (remote_meta_ptr->consumer + 1) % MSGI_MAX_SBP_BLOCKS_PER_BUFFER;
            remote_meta_ptr->consumer = index;

            p += block_size;
        }

        // Check if the shared memory queue is not empty
        while(remote_meta_ptr->consumer == remote_meta_ptr->producer){}

        memcpy((void *)p, GET_ADDR(remote_buffer,remote_meta_ptr->consumer,block_size), sizeb - (bcount - 1) * block_size);

        index = (remote_meta_ptr->consumer + 1) % MSGI_MAX_SBP_BLOCKS_PER_BUFFER;
        remote_meta_ptr->consumer = index;

        //mark the entry to FREE
        remote_meta_ptr->control_flag = FREE;
    }
    else
    {
        //Check if the shared memory queue is not empty
        while(remote_meta_ptr->consumer == remote_meta_ptr->producer){}

        memcpy(data,GET_ADDR(remote_buffer,remote_meta_ptr->consumer,block_size),sizeb);

        index = (remote_meta_ptr->consumer + 1) % MSGI_MAX_SBP_BLOCKS_PER_BUFFER;

        remote_meta_ptr->consumer = index;

        //mark the entry to FREE
        remote_meta_ptr->control_flag = FREE;
    }

    MSG_DBG(level, MESSAGE, "(after)consumer = %d", remote_meta_ptr->consumer);
    MSG_DBG(level, MESSAGE, "Leaving msgmi_copy_from_sbp() ...");

    return MSG_SUCCESS;
}

//  =====   INTERNAL FUNCTIONS  =====
static inline unsigned int msgi_count_blocks(unsigned int size)
{
    return count_blocks(size, block_size);        
}

//  =====   CONSTRUCTOR and DESTRUCTOR  =====
MSG_ERR_T msgi_sbp_init()
{
    int i,j;
    volatile msgi_sbp_meta_t *p = NULL;

    msgi_rq_meta_size       =   sizeof(msgi_RQ_meta_t);
    msgi_rq_buffer_size     =   MSGI_MAX_RQE * MSGI_MAX_RQE_SIZE;
    msgi_sbp_meta_size      =   (sizeof(msgi_sbp_meta_t) * MSGI_MAX_SBP_BUFFERS);
    //msgi_sbp_pool_size      =   1048576;       //1MB
    msgi_sbp_pool_size      =   MSGI_SBP_BUFFER_SIZE * MSGI_MAX_SBP_BUFFERS;    //buffer_size * buffers
    //buffer_size = msgi_sbp_pool_size / MSGI_MAX_SBP_BUFFERS;
    buffer_size = MSGI_SBP_BUFFER_SIZE;
    block_size  = MSGI_SBP_BLOCK_SIZE;

#ifdef SERVICE_PROCESSOR
    msgi_rq_meta_start = service_processor_shm_address_location + (local_rank * (sizeof(msgi_RQ_meta_t)));
    msgi_rq_buffer_start = service_processor_shm_address_location + (ENV_CORE_NUM * sizeof(msgi_RQ_meta_t)) + (local_rank * msgi_rq_buffer_size);
    msgi_rfq_meta_start = service_processor_shm_address_location + (ENV_CORE_NUM * sizeof(msgi_RQ_meta_t)) + (ENV_CORE_NUM * msgi_rq_buffer_size) + (local_rank * sizeof(msgi_RQ_meta_t));
    msgi_rfq_buffer_start = service_processor_shm_address_location + (ENV_CORE_NUM * sizeof(msgi_RQ_meta_t)) + (ENV_CORE_NUM * msgi_rq_buffer_size) + (ENV_CORE_NUM * sizeof(msgi_RQ_meta_t)) + (local_rank * msgi_rq_buffer_size);
    msgi_sbp_meta_start = service_processor_shm_address_location + (ENV_CORE_NUM * sizeof(msgi_RQ_meta_t)) + (ENV_CORE_NUM * msgi_rq_buffer_size) + (ENV_CORE_NUM * sizeof(msgi_RQ_meta_t)) + (ENV_CORE_NUM * msgi_rq_buffer_size) + (local_rank * msgi_sbp_meta_size);
    msgi_sbp_buffer_start = service_processor_shm_address_location + (ENV_CORE_NUM * sizeof(msgi_RQ_meta_t)) + (ENV_CORE_NUM * msgi_rq_buffer_size) + (ENV_CORE_NUM * sizeof(msgi_RQ_meta_t)) + (ENV_CORE_NUM * msgi_rq_buffer_size) + (ENV_CORE_NUM * msgi_sbp_meta_size) + (local_rank * msgi_sbp_pool_size);

    for(i=0;i<ENV_CORE_NUM;i++)
    {
        for(j=0;j<MSGI_MAX_SBP_BUFFERS;j++)
        {
            msgi_sbp_meta[i][j] =  service_processor_shm_address_location + ((ENV_CORE_NUM * sizeof(msgi_RQ_meta_t)) + (ENV_CORE_NUM * msgi_rq_buffer_size))*2 + i * msgi_sbp_meta_size + j * sizeof(msgi_sbp_meta_t);
            msgi_sbp[i][j] = service_processor_shm_address_location + ((ENV_CORE_NUM * sizeof(msgi_RQ_meta_t)) + (ENV_CORE_NUM * msgi_rq_buffer_size))*2 + (ENV_CORE_NUM * msgi_sbp_meta_size) + i * msgi_sbp_pool_size + j * buffer_size;

            //msgi_sbp
            p = (msgi_sbp_meta_t *) (uintptr_t)msgi_sbp_meta[i][j];
            p->producer = 0;
            p->consumer = 0;           
            p->control_flag = FREE;
        }
    }
#else
    msgi_rq_meta_start      =   shm_address_location + (local_rank * (ENV_CORE_NUM-1) * (sizeof(msgi_RQ_meta_t)));
    msgi_rq_buffer_start    =   shm_address_location + (ENV_CORE_NUM * (ENV_CORE_NUM-1) * (sizeof(msgi_RQ_meta_t))) + (local_rank * (ENV_CORE_NUM-1) * msgi_rq_buffer_size);
    msgi_sbp_meta_start     =   shm_address_location + (ENV_CORE_NUM * (ENV_CORE_NUM-1) * (sizeof(msgi_RQ_meta_t) + msgi_rq_buffer_size)) + local_rank * msgi_sbp_meta_size;
    msgi_sbp_buffer_start   =   shm_address_location + (ENV_CORE_NUM * (ENV_CORE_NUM-1) * (sizeof(msgi_RQ_meta_t) + msgi_rq_buffer_size)) + (ENV_CORE_NUM * msgi_sbp_meta_size) + local_rank * msgi_sbp_pool_size;


    for(i=0;i<ENV_CORE_NUM;i++)
    {
        for(j=0;j<MSGI_MAX_SBP_BUFFERS;j++)
        {
            msgi_sbp_meta[i][j] =  shm_address_location + (ENV_CORE_NUM * (ENV_CORE_NUM-1) * (sizeof(msgi_RQ_meta_t) + msgi_rq_buffer_size)) + i * msgi_sbp_meta_size + j * sizeof(msgi_sbp_meta_t);
            msgi_sbp[i][j] = shm_address_location + (ENV_CORE_NUM * (ENV_CORE_NUM-1) * (sizeof(msgi_RQ_meta_t) + msgi_rq_buffer_size)) + (ENV_CORE_NUM * msgi_sbp_meta_size) + i * msgi_sbp_pool_size + j * buffer_size;

            //msgi_sbp
            p = (msgi_sbp_meta_t *) (uintptr_t)msgi_sbp_meta[i][j];
            p->producer = 0;
            p->consumer = 0;           
            p->control_flag = FREE;
        }
    }
#endif
/* 
       if(!local_rank)
       {
       for(i=0;i<ENV_CORE_NUM;i++)
       {
       for(j=0;j<MSGI_MAX_SBP_BUFFERS;j++)
       {
       printf("msgi_sbp_meta[%d][%d] = %ld\n",i,j,msgi_sbp_meta[i][j]);
       printf("msgi_sbp[%d][%d] = %ld\n",i,j,msgi_sbp[i][j]);
       }
       }
       }

       if(!local_rank)
       {
       fprintf(stderr,"[%d]###################################\n",local_rank);
       fprintf(stderr,"[%d]msgi_rq_meta_size: %d\n",local_rank,msgi_rq_meta_size);
       fprintf(stderr,"[%d]msgi_rq_buffer_size: %d\n",local_rank,msgi_rq_buffer_size);
       fprintf(stderr,"[%d]msgi_sbp_meta_size: %d\n",local_rank,msgi_sbp_meta_size);
       fprintf(stderr,"[%d]msgi_sbp_pool_size: %d\n",local_rank,msgi_sbp_pool_size);
       fprintf(stderr,"[%d]msgi_rq_meta_start: %ld\n",local_rank,msgi_rq_meta_start);
       fprintf(stderr,"[%d]msgi_rq_buffer_start: %ld\n",local_rank,msgi_rq_buffer_start);
       fprintf(stderr,"[%d]msgi_sbp_meta_start: %ld\n",local_rank,msgi_sbp_meta_start);
       fprintf(stderr,"[%d]msgi_sbp_buffer_start: %ld\n",local_rank,msgi_sbp_buffer_start);
       fprintf(stderr,"[%d]###################################\n",local_rank);
       }
*/

    return MSG_SUCCESS;
}

MSG_ERR_T msgi_sbp_exit()
{
    return MSG_SUCCESS;
}
