/*
 * Copyright (C) 2011 PASLab CSIE NTU. All rights reserved.
 *      - Chiu Po-Hsun <maxshow0906@gmail.com>
 */

#include <pthread.h>
#include "msg_internal.h"
#include "msgi_q.h"

extern int local_rank;

long service_processor_shm_address_location;

// ---- Local prototypes ----
static inline MSG_ERR_T msgi_get_local_msg(int rank);
static inline MSG_ERR_T msgi_post_remote_msg(int rank, void *,int );
static inline void msgi_dump_rq(int n);

// ---- Locals ----
static long msgi_local_RFQ_meta[ENV_CORE_NUM];
static long msgi_local_RFQe[ENV_CORE_NUM];
static long msgi_remote_RFQ_meta[ENV_CORE_NUM];
static long msgi_remote_RFQe[ENV_CORE_NUM];

// ---- Globals ----
pthread_t service_processor;
//int service_processor_control_flag=INIT;


static void *msgi_service_processor_create(void *args)
{
    int i, level=3;
    MSG_DBG(level, MESSAGE, "In msgi_service_processor_create ...");

    //enable async cancellation
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    while(1)
    {
        for(i=0;i<ENV_CORE_NUM;i++)
            msgi_get_local_msg(i);
        /*
           if(service_processor_control_flag == TERMINATE)
           {
           printf("value: %d\n",service_processor_control_flag);
           break;
           }
         */
        //sleep(1);
        //printf("OK\n");
    }

    MSG_DBG(level, MESSAGE, "Leaving msgi_service_processor_create ...");

    //printf("pthread_exit\n");
    pthread_exit(NULL);
    //printf("pthread_exit\n");
}

// ====   INTERNAL FUNCTIONS ====
static inline MSG_ERR_T msgi_get_local_msg(int src_rank)
{
    volatile msgi_RQ_meta_t *meta_ptr = (msgi_RQ_meta_t *)(uintptr_t)msgi_local_RFQ_meta[src_rank];
    volatile msgi_RQe_t *localQ_ptr = NULL;
    volatile msgi_RQe_t *msg_info = NULL;
    int index,rc;
    int level = 3;

    MSG_DBG(level, MESSAGE, "In msgi_get_local_msg() ..."); 

    MSG_DBG(level, MESSAGE, "producer address : %ld", (long)(&(meta_ptr->producer)));
    MSG_DBG(level, MESSAGE, "consumer address : %ld", (long)(&(meta_ptr->consumer)));
    MSG_DBG(level, MESSAGE, "producer = %d", meta_ptr->producer);
    MSG_DBG(level, MESSAGE, "consumer = %d", meta_ptr->consumer);

    // Queue is empty
    if (meta_ptr->consumer == meta_ptr->producer)
        return MSG_TRANS_NOT_READY;
    else
    {
        index = meta_ptr->consumer;                                                             

        MSG_DBG(level, MESSAGE, "Getting message ...");
        MSG_DBG(level, MESSAGE, "consumer = %d", meta_ptr->consumer);

        localQ_ptr = (msgi_RQe_t *)(uintptr_t)(msgi_local_RFQe[src_rank] + index * MSGI_MAX_RQE_SIZE);

        msg_info = (msgi_RQe_t *)localQ_ptr;

        if(msg_info->len <= MSGI_MAX_RQE_SIZE)
        {
            //copy two entries to dst_rank
            do{
            rc = msgi_post_remote_msg(localQ_ptr->rid,(void *)localQ_ptr,MSGI_MAX_RQE_SIZE*2);
            }while(rc != MSG_SUCCESS);

            index = (index + 2) % MSGI_MAX_RQE;
            meta_ptr->consumer = index;
        }
        else
        {
            //copy one entry to dst_rank
            do{
            rc = msgi_post_remote_msg(localQ_ptr->rid,(void *)localQ_ptr,MSGI_MAX_RQE_SIZE);
            }while(rc != MSG_SUCCESS);

            index = (index + 1) % MSGI_MAX_RQE;
            meta_ptr->consumer = index;
        }
    }

    MSG_DBG(level, MESSAGE, "Leaving msgi_get_local_msg() ...");

    return MSG_SUCCESS;
}

static inline MSG_ERR_T msgi_post_remote_msg(int dst_rank, void * msg, int size)
{
    volatile msgi_RQ_meta_t *meta_ptr = (msgi_RQ_meta_t *)(uintptr_t)msgi_remote_RFQ_meta[dst_rank];
    volatile long *remoteQ_ptr = NULL;
    int index = 0;
    int level = 3;

    MSG_DBG(level, MESSAGE, "In msgmi_post_remote_msg() ...");
    MSG_DBG(level, MESSAGE, "Target: %d", dst_rank);
    MSG_DBG(level, MESSAGE, "producer address: %ld", (long)(&(meta_ptr->producer)));
    MSG_DBG(level, MESSAGE, "consumer address: %ld", (long)(&(meta_ptr->consumer)));

    MSG_DBG(level, MESSAGE, "producer = %d", meta_ptr->producer);
    MSG_DBG(level, MESSAGE, "consumer = %d", meta_ptr->consumer);

    //check the number of entries
    if(size == MSGI_MAX_RQE_SIZE)
        index = (meta_ptr->producer + 1) % MSGI_MAX_RQE;
    else
        index = (meta_ptr->producer + 2) % MSGI_MAX_RQE;

    MSG_DBG(level, MESSAGE, "index = %d", index);

    // Queue is full        
    if (index == meta_ptr->consumer) 
    {
        MSG_DBG(level, MESSAGE, "Leaving msgmi_post_remote_msg() ... (Queue is full)");
        return MSG_TRANS_NOT_READY;  
    } 
    else 
    {
        remoteQ_ptr = (long *)(uintptr_t)(msgi_remote_RFQe[dst_rank] + meta_ptr->producer * MSGI_MAX_RQE_SIZE);

        memcpy((void *)remoteQ_ptr, msg, size);

        MSG_DBG(level, MESSAGE, "remoteQ_ptr = %d", ((msgi_RQe_t *)remoteQ_ptr)->len);
        MSG_DBG(level, MESSAGE, "#####################################################################");

        meta_ptr->producer = index;
    }

    MSG_DBG(level, MESSAGE, "Leaving msgmi_post_remote_msg() ...");

    return MSG_SUCCESS;
}

static void msgi_dump_rq(int n)
{
    int i, level = n;

    MSG_DBG(level, MESSAGE, "Address information of the RFQ:");

    for (i = 0; i < ENV_CORE_NUM; i++) {
        MSG_DBG(level, MESSAGE, "msgmi_local_RFQ_meta[%d]: %ld", i, msgi_local_RFQ_meta[i]);
        MSG_DBG(level, MESSAGE, "msgmi_local_RFQe[%d]: %ld", i, msgi_local_RFQe[i]);

        MSG_DBG(level, MESSAGE, "msgmi_remote_RFQ_meta[%d]: %ld", i, msgi_remote_RFQ_meta[i]);
        MSG_DBG(level, MESSAGE, "msgmi_remote_RFQe[%d]: %ld", i, msgi_remote_RFQe[i]);
    }
}

// ==== CONSTRUCTOR and DESTRUCTOR ====
MSG_ERR_T msgi_service_processor_init(void *argvp, void *envp)
{
    MSG_ERR_T rc;
    long rq_meta_base,rq_buffer_base;
    long rfq_meta_base,rfq_buffer_base;
    msgi_RQ_meta_t *p = NULL;
    int i;
    int level = 3;

    MSG_DBG(level, MESSAGE, "In msgi_service_processor_init ...");

    rq_meta_base = service_processor_shm_address_location; 
    rq_buffer_base = rq_meta_base + (ENV_CORE_NUM * sizeof(msgi_RQ_meta_t));
    rfq_meta_base = rq_buffer_base + (ENV_CORE_NUM * MSGI_MAX_RQE * MSGI_MAX_RQE_SIZE);
    rfq_buffer_base = rfq_meta_base + (ENV_CORE_NUM * sizeof(msgi_RQ_meta_t)); 

    for(i=0;i<ENV_CORE_NUM;i++)
    {
        msgi_local_RFQ_meta[i] =    rfq_meta_base;
        msgi_local_RFQe[i] =        rfq_buffer_base;
        msgi_remote_RFQ_meta[i] =   rq_meta_base;
        msgi_remote_RFQe[i] =       rq_buffer_base;

        p = (msgi_RQ_meta_t *)(uintptr_t)msgi_local_RFQ_meta[i];
        p->producer = 0;
        p->consumer = 0;
        p = (msgi_RQ_meta_t *)(uintptr_t)msgi_remote_RFQ_meta[i];
        p->producer = 0;
        p->consumer = 0;

        rfq_meta_base +=        sizeof(msgi_RQ_meta_t);
        rfq_buffer_base +=      MSGI_MAX_RQE * MSGI_MAX_RQE_SIZE;
        rq_meta_base +=         sizeof(msgi_RQ_meta_t);
        rq_buffer_base +=       MSGI_MAX_RQE * MSGI_MAX_RQE_SIZE;
    }

    //dump the message
    //msgi_dump_rq(1);

    rc = pthread_create(&service_processor,NULL,msgi_service_processor_create,NULL);
   // printf("rc: %d\n",rc);
    if(rc)
        return MSG_ERR_NO_RESOURCE;

    MSG_DBG(level, MESSAGE, "leaving msgi_service_processor_init()");

    return MSG_SUCCESS;
}

MSG_ERR_T msgi_service_processor_exit(void *argvp, void *envp)
{
    //test
    //printf("[%d]service_processor_exit PID: %d\n",local_rank,(int)getpid());

    //service_processor_control_flag = TERMINATE;

    //cancel the service processor
    pthread_cancel(service_processor);
    pthread_join(service_processor,NULL);

    //printf("main process join the service processor\n");

    return MSG_SUCCESS;
}
