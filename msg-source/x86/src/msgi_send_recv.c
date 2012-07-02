/* --------------------------------------------------------------  */
/* (C)Copyright 2011                                               */
/* Po-Hsun Chiu <maxshow0906@gmail.com>                        */
/* All Rights Reserved.                                            */                                                         
/*                                                                 */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND          */
/* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,     */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF        */
/* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE        */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR            */
/* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,    */
/* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT    */
/* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;    */
/* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)        */
/* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN       */
/* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR    */
/* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  */
/* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              */
/* --------------------------------------------------------------  */

/*C header-----------------------*/
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
/*-------------------------------*/

/*MSG header---------------------*/
#include "msg.h"
#include "msg_internal.h"
#include "msg_point-to-point.h"
#include "msgi_q.h"
#include "msgi_sbp.h"
/*-------------------------------*/


extern char *shm_address;
extern long shm_address_location;
extern int local_rank;
extern int numpros;
/*-------------------------------*/

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

extern long msgi_sbp_meta[ENV_CORE_NUM][MSGI_MAX_SBP_BUFFERS];
extern long msgi_sbp[ENV_CORE_NUM][MSGI_MAX_SBP_BUFFERS];
/*-------------------------------*/

// ---- Locals ----
#ifdef SERVICE_PROCESSOR
static long msgi_local_RQ_meta;
static long msgi_local_RQe;
static long msgi_remote_RQ_meta;
static long msgi_remote_RQe;
#else
static long msgi_local_RQ_meta[ENV_CORE_NUM];
static long msgi_local_RQe[ENV_CORE_NUM];
static long msgi_remote_RQ_meta[ENV_CORE_NUM];
static long msgi_remote_RQe[ENV_CORE_NUM];
#endif

/*internal functions-------------*/
static inline MSG_ERR_T msgi_get_local_msg(int rank,volatile void *OUT_msg,int size,msgi_RQe_t *msg_info);
static inline MSG_ERR_T msgi_post_remote_msg(int rank,volatile void *msg,int size,msgi_RQe_t *msg_info,int scheme);
static inline int msgi_sbp_get_free_buffer();
static inline MSG_ERR_T msgi_send(void *src_data,int sizeb,int dst_rank,int tag,MSG_Comm comm);
static inline MSG_ERR_T msgi_recv(void *dst_data,int sizeb,int src_rank,int tag,MSG_Comm comm,MSG_Status *status);
/*-------------------------------*/

MSG_ERR_T msgi_send_recv(void *data,int count,MSG_Datatype data_type,int rank,int tag,MSG_Comm comm,MSG_Status *status,msgi_msg_type_t communication_type)
{
    MSG_ERR_T rc = MSG_SUCCESS;
    int sizeb=0;
    int data_type_sizeb=0;

    data_type_sizeb = check_data_type(data_type);
    sizeb = count*data_type_sizeb;

    if(communication_type == MSGI_MSG_SEND)
    {
        rc = msgi_send(data,sizeb,rank,tag,comm);
    }
    else if(communication_type == MSGI_MSG_RECV)
    {
        rc = msgi_recv(data,sizeb,rank,tag,comm,status);
    }

    return rc;
}

static inline MSG_ERR_T msgi_send(void *src_data,int sizeb,int dst_rank,int tag,MSG_Comm comm)
{
    MSG_ERR_T rc;

    volatile msgi_RQe_t *msg;
    volatile msgi_RQe_t msgi_msgbuf;
    int level = 3;

#ifdef MSG_ERROR_CHECKING
    switch(data_type)
    {
        case MSG_CHAR:
            rc = comm_input_check(dst_rank,src_data,sizeb,tag);
            break;
        default:
            break;
    }
#endif

    //error check for rank
    if(dst_rank < 0 || dst_rank >= numpros)
        return MSG_ERR_INVALID_NODE;

    // Optimization for samll and large size
    if(sizeb <= MSGI_MAX_RQE_SIZE)
    {
        msg = &msgi_msgbuf;

        msg->len = sizeb;
        msg->tag = tag;
        msg->sid = local_rank;
        msg->rid = dst_rank;

        MSG_DBG(level,MESSAGE,"[%d]Copying data to remote RQ ...",local_rank);
        do{
            rc = msgi_post_remote_msg(dst_rank,src_data,sizeb,(msgi_RQe_t *)msg,SMALL);
        }while(rc != MSG_SUCCESS);
    }
    else
    {
        msg = &msgi_msgbuf;

        //get the buf_idx
        msg->index = msgi_sbp_get_free_buffer();
        msg->len = sizeb;
        msg->tag = tag;
        msg->sid = local_rank;
        msg->rid = dst_rank;

        //Posting request to remote RQ
        MSG_DBG(level, MESSAGE, "[%d]Posting request to remote RQ ...",local_rank);
        do{
            //rc = msgi_post_remote_msg(dst_rank,msg,sizeof(msgi_RQe_t));
            rc = msgi_post_remote_msg(dst_rank,msg,sizeof(msgi_RQe_t),NULL,LARGE);
        } while(rc != MSG_SUCCESS);

        //moving data to relay buffer
        MSG_DBG(level, MESSAGE, "[%d]Copying data (size = %d) to local SBP[%d]...",local_rank, sizeb, msg->index);
        msgi_copy_to_sbp(msg->index,src_data,sizeb);
    }

    MSG_DBG(level, MESSAGE, "[%d]End of send routine ...",local_rank);

    return MSG_SUCCESS;
}

static inline MSG_ERR_T msgi_recv(void *dst_data,int sizeb,int src_rank,int tag,MSG_Comm comm,MSG_Status *status)
{
    MSG_ERR_T rc;

    volatile msgi_RQe_t msgi_msgbuf;
    int level = 3;

#ifdef MSG_ERROR_CHECKING
    switch(data_type)
    {
        case MSG_CHAR:
            rc = comm_input_check(src_rank,dst_data,sizeb,tag);
            break;
        default:
            return MSG_ERR_DATA_TYPE;
            break;
    }
#endif

    //error check for rank
    if(src_rank < 0 || src_rank >= numpros)
        return MSG_ERR_INVALID_NODE;

    // Optimization for samll and large size
    MSG_DBG(level, MESSAGE, "[%d]Copying data from local RQ ...",local_rank);
    do{
        rc = msgi_get_local_msg(src_rank,dst_data,sizeb,(msgi_RQe_t *)&msgi_msgbuf);
    }while (rc != MSG_SUCCESS);

    return MSG_SUCCESS;
}

//  =====   INTERNAL FUNCTIONS  ===============
static inline MSG_ERR_T msgi_get_local_msg(int rank,volatile void *OUT_msg,int size,msgi_RQe_t *msg_info)
{
#ifdef SERVICE_PROCESSOR
    volatile msgi_RQ_meta_t *meta_ptr = (msgi_RQ_meta_t *) (uintptr_t)msgi_local_RQ_meta;
    volatile long *localRQe_ptr = NULL;
    volatile int index = 0;
    int level = 3;

    MSG_DBG(level, MESSAGE, "[%d]In msgmi_get_local_msg() ...",local_rank);

    MSG_DBG(level, MESSAGE, "[%d]producer address : %ld",local_rank, (long)(&(meta_ptr->producer)));
    MSG_DBG(level, MESSAGE, "[%d]consumer address : %ld",local_rank, (long)(&(meta_ptr->consumer)));
    MSG_DBG(level, MESSAGE, "[%d]Before producer = %d",local_rank, meta_ptr->producer);
    MSG_DBG(level, MESSAGE, "[%d]Before consumer = %d",local_rank, meta_ptr->consumer);

    if(meta_ptr->consumer == meta_ptr->producer)
    {
        return MSG_TRANS_NOT_READY;
    }
    else
    {
        index = meta_ptr->consumer;

        MSG_DBG(level, MESSAGE, "[%d]Getting message ...",local_rank);
        MSG_DBG(level, MESSAGE, "[%d]consumer = %d",local_rank, meta_ptr->consumer);

        localRQe_ptr = (long *)(uintptr_t) (msgi_local_RQe + index * MSGI_MAX_RQE_SIZE);

        msg_info = (msgi_RQe_t *)localRQe_ptr;

        if(msg_info->len <= MSGI_MAX_RQE_SIZE)
        {
            localRQe_ptr = (long *)(uintptr_t) (msgi_local_RQe + ((index + 1) % MSGI_MAX_RQE) * MSGI_MAX_RQE_SIZE);

            memcpy((void *)OUT_msg,(void *)localRQe_ptr,msg_info->len);

            MSG_DBG(level, MESSAGE, "[%d]Copy the request queue info from local_RQ[%d] to OUT_msg finished ...",local_rank,rank);

            index = (index + 2) % MSGI_MAX_RQE;
            meta_ptr->consumer = index;
        }
        else
        {
            msgi_copy_from_sbp(rank,msg_info->index,(void *)OUT_msg,msg_info->len);

            MSG_DBG(level, MESSAGE, "[%d]Copy the request queue info from local_RQ[%d] to OUT_msg finished ...",local_rank,rank);

            index = (index + 1) % MSGI_MAX_RQE;
            meta_ptr->consumer = index;
        }
    }
#else
    volatile msgi_RQ_meta_t *meta_ptr = (msgi_RQ_meta_t *) (uintptr_t)msgi_local_RQ_meta[rank];
    volatile long *localRQe_ptr = NULL;
    volatile int index = 0;
    int level = 3;

    MSG_DBG(level, MESSAGE, "[%d]In msgmi_get_local_msg() ...",local_rank);

    MSG_DBG(level, MESSAGE, "[%d]producer address : %ld",local_rank, (long)(&(meta_ptr->producer)));
    MSG_DBG(level, MESSAGE, "[%d]consumer address : %ld",local_rank, (long)(&(meta_ptr->consumer)));
    MSG_DBG(level, MESSAGE, "[%d]Before producer = %d",local_rank, meta_ptr->producer);
    MSG_DBG(level, MESSAGE, "[%d]Before consumer = %d",local_rank, meta_ptr->consumer);

    if(meta_ptr->consumer == meta_ptr->producer)
    {
        return MSG_TRANS_NOT_READY;
    }
    else
    {
        index = meta_ptr->consumer;

        MSG_DBG(level, MESSAGE, "[%d]Getting message ...",local_rank);
        MSG_DBG(level, MESSAGE, "[%d]consumer = %d",local_rank, meta_ptr->consumer);

        localRQe_ptr = (long *)(uintptr_t) (msgi_local_RQe[rank] + index * MSGI_MAX_RQE_SIZE);

        msg_info = (msgi_RQe_t *)localRQe_ptr;

        if(msg_info->len <= MSGI_MAX_RQE_SIZE)
        {
            localRQe_ptr = (long *)(uintptr_t) (msgi_local_RQe[rank] + ((index + 1) % MSGI_MAX_RQE) * MSGI_MAX_RQE_SIZE);

            memcpy((void *)OUT_msg,(void *)localRQe_ptr,msg_info->len);

            MSG_DBG(level, MESSAGE, "[%d]Copy the request queue info from local_RQ[%d] to OUT_msg finished ...",local_rank,rank);

            index = (index + 2) % MSGI_MAX_RQE;
            meta_ptr->consumer = index;
        }
        else
        {
            msgi_copy_from_sbp(rank,msg_info->index,(void *)OUT_msg,msg_info->len);

            MSG_DBG(level, MESSAGE, "[%d]Copy the request queue info from local_RQ[%d] to OUT_msg finished ...",local_rank,rank);

            index = (index + 1) % MSGI_MAX_RQE;
            meta_ptr->consumer = index;
        }
    }
#endif
    MSG_DBG(level, MESSAGE, "[%d]After producer = %d",local_rank, meta_ptr->producer);
    MSG_DBG(level, MESSAGE, "[%d]After consumer = %d",local_rank, meta_ptr->consumer);
    MSG_DBG(level, MESSAGE, "[%d]Leaving msgmi_get_local_msg() ...",local_rank);

    return MSG_SUCCESS;
}

static inline MSG_ERR_T msgi_post_remote_msg(int rank,volatile void *msg,int size,msgi_RQe_t *msg_info,int scheme)
{
#ifdef SERVICE_PROCESSOR
    volatile msgi_RQ_meta_t *meta_ptr = (msgi_RQ_meta_t *) (uintptr_t)msgi_remote_RQ_meta;
    volatile long *remoteRQe_ptr = NULL;
    volatile int index = 0;
    int level = 3;

    MSG_DBG(level, MESSAGE, "[%d]In msgmi_post_remote_msg() ...",local_rank);
    MSG_DBG(level, MESSAGE, "[%d]Target: %d",local_rank,rank);
    MSG_DBG(level, MESSAGE, "[%d]producer address: %ld",local_rank, (long)(&(meta_ptr->producer)));
    MSG_DBG(level, MESSAGE, "[%d]consumer address: %ld",local_rank, (long)(&(meta_ptr->consumer)));

    MSG_DBG(level, MESSAGE, "[%d]Before producer = %d",local_rank, meta_ptr->producer);
    MSG_DBG(level, MESSAGE, "[%d]Before consumer = %d",local_rank, meta_ptr->consumer);

    switch(scheme)
    {
        case SMALL:
            index = (meta_ptr->producer + 2) % MSGI_MAX_RQE;

            //Queue is full
            if(index == meta_ptr->consumer)
            {
                MSG_DBG(level, MESSAGE, "[%d]Leaving msgmi_post_remote_msg() ... (Queue is full)",local_rank);
                return MSG_TRANS_NOT_READY;
            }
            else
            {
                //sreach the request queue's entry
                remoteRQe_ptr = (long *) (uintptr_t)(msgi_remote_RQe + meta_ptr->producer * MSGI_MAX_RQE_SIZE);

                MSG_DBG(level, MESSAGE, "[%d]remote RQe addr : %ld",local_rank, (long)remoteRQe_ptr);

                //copy msg info to queue
                memcpy((void *)remoteRQe_ptr,(void *)msg_info,sizeof(msgi_RQe_t));

                remoteRQe_ptr = (long *) (uintptr_t)(msgi_remote_RQe + ((meta_ptr->producer + 1) % MSGI_MAX_RQE) * MSGI_MAX_RQE_SIZE);
                //copy real data to queue
                memcpy((void *)remoteRQe_ptr,(void *)msg,size);

                meta_ptr->producer = index;
            }
            break;
        case LARGE:
            index = (meta_ptr->producer + 1) % MSGI_MAX_RQE;

            //Queue is full
            if(index == meta_ptr->consumer)
            {
                MSG_DBG(level, MESSAGE, "[%d]Leaving msgmi_post_remote_msg() ... (Queue is full)",local_rank);
                return MSG_TRANS_NOT_READY;
            }
            else
            {
                //sreach the request queue's entry
                remoteRQe_ptr = (long *) (uintptr_t)(msgi_remote_RQe + meta_ptr->producer * MSGI_MAX_RQE_SIZE);

                MSG_DBG(level, MESSAGE, "[%d]remote RQe addr : %ld",local_rank, (long)remoteRQe_ptr);

                memcpy((void *)remoteRQe_ptr,(void *)msg,size);

                meta_ptr->producer = index;
            }
            break;
        default:
            break;
    }
#else
    volatile msgi_RQ_meta_t *meta_ptr = (msgi_RQ_meta_t *) (uintptr_t)msgi_remote_RQ_meta[rank];
    volatile long *remoteRQe_ptr = NULL;
    volatile int index = 0;
    int level = 3;

    MSG_DBG(level, MESSAGE, "[%d]In msgmi_post_remote_msg() ...",local_rank);
    MSG_DBG(level, MESSAGE, "[%d]Target: %d",local_rank,rank);
    MSG_DBG(level, MESSAGE, "[%d]producer address: %ld",local_rank, (long)(&(meta_ptr->producer)));
    MSG_DBG(level, MESSAGE, "[%d]consumer address: %ld",local_rank, (long)(&(meta_ptr->consumer)));

    MSG_DBG(level, MESSAGE, "[%d]Before producer = %d",local_rank, meta_ptr->producer);
    MSG_DBG(level, MESSAGE, "[%d]Before consumer = %d",local_rank, meta_ptr->consumer);

    switch(scheme)
    {
        case SMALL:

            index = (meta_ptr->producer + 2) % MSGI_MAX_RQE;

            //Queue is full
            if(index == meta_ptr->consumer)
            {
                MSG_DBG(level, MESSAGE, "[%d]Leaving msgmi_post_remote_msg() ... (Queue is full)",local_rank);
                return MSG_TRANS_NOT_READY;
            }
            else
            {
                //sreach the request queue's entry
                remoteRQe_ptr = (long *) (uintptr_t)(msgi_remote_RQe[rank] + meta_ptr->producer * MSGI_MAX_RQE_SIZE);

                MSG_DBG(level, MESSAGE, "[%d]remote RQe addr : %ld",local_rank, (long)remoteRQe_ptr);

                //copy msg info to queue
                memcpy((void *)remoteRQe_ptr,(void *)msg_info,sizeof(msgi_RQe_t));

                remoteRQe_ptr = (long *) (uintptr_t)(msgi_remote_RQe[rank] + ((meta_ptr->producer + 1) % MSGI_MAX_RQE) * MSGI_MAX_RQE_SIZE);
                //copy real data to queue
                memcpy((void *)remoteRQe_ptr,(void *)msg,size);

                meta_ptr->producer = index;
            }
            break;
        case LARGE:
            index = (meta_ptr->producer + 1) % MSGI_MAX_RQE;

            //Queue is full
            if(index == meta_ptr->consumer)
            {
                MSG_DBG(level, MESSAGE, "[%d]Leaving msgmi_post_remote_msg() ... (Queue is full)",local_rank);
                return MSG_TRANS_NOT_READY;
            }
            else
            {
                //sreach the request queue's entry
                remoteRQe_ptr = (long *) (uintptr_t)(msgi_remote_RQe[rank] + meta_ptr->producer * MSGI_MAX_RQE_SIZE);

                MSG_DBG(level, MESSAGE, "[%d]remote RQe addr : %ld",local_rank, (long)remoteRQe_ptr);

                memcpy((void *)remoteRQe_ptr,(void *)msg,size);

                meta_ptr->producer = index;
            }
            break;
        default:
            break;
    }
#endif
    MSG_DBG(level, MESSAGE, "[%d]After producer = %d",local_rank, meta_ptr->producer);
    MSG_DBG(level, MESSAGE, "[%d]After consumer = %d",local_rank, meta_ptr->consumer);
    MSG_DBG(level, MESSAGE, "[%d]Leaving msgmi_post_remote_msg() ...",local_rank);

    return MSG_SUCCESS;
}

static inline int msgi_sbp_get_free_buffer()
{
    volatile msgi_sbp_meta_t *meta_ptr = NULL;
    volatile int buf_idx = 0;

    meta_ptr = (msgi_sbp_meta_t *) (uintptr_t) msgi_sbp_meta[local_rank][0];

    while(1)
    {
        if(meta_ptr->control_flag == FREE)
            return buf_idx;
        else
        {
            buf_idx = (buf_idx+1) % MSGI_MAX_SBP_BUFFERS;
            meta_ptr = (msgi_sbp_meta_t *) (uintptr_t) msgi_sbp_meta[local_rank][buf_idx];
        }
    }
}

//  =====   CONSTRUCTOR and DESTRUCTOR  =======
MSG_ERR_T msgi_send_recv_init(void *argc,void *argv)
{
    long rq_meta_base,rq_buffer_base;
    volatile msgi_RQ_meta_t *p = NULL;

    //rq_meta_base = shm_address_location + (local_rank * (ENV_CORE_NUM-1) * (sizeof(msgi_RQ_meta_t)));
    //rq_buffer_base = shm_address_location + (ENV_CORE_NUM * (ENV_CORE_NUM-1) * (sizeof(msgi_RQ_meta_t))) + (local_rank * (ENV_CORE_NUM-1) * msgi_rq_buffer_size);
    rq_meta_base = msgi_rq_meta_start;
    rq_buffer_base = msgi_rq_buffer_start;

#ifdef SERVICE_PROCESSOR
    long rfq_meta_base,rfq_buffer_base;
    rfq_meta_base = msgi_rfq_meta_start;
    rfq_buffer_base = msgi_rfq_buffer_start;

    msgi_local_RQ_meta = rq_meta_base;
    msgi_local_RQe = rq_buffer_base;
    p = (msgi_RQ_meta_t *) (uintptr_t)msgi_local_RQ_meta;
    p->producer = 0;
    p->consumer = 0;

    msgi_remote_RQ_meta = rfq_meta_base;
    msgi_remote_RQe = rfq_buffer_base;

    p = (msgi_RQ_meta_t *) (uintptr_t)msgi_remote_RQ_meta;
    p->producer = 0;
    p->consumer = 0;

/*
    sleep(local_rank+1);
    fprintf(stderr,"[%d]###################################\n",local_rank);
    fprintf(stderr,"[%d]msgi_local_RQ_meta: %ld\n",local_rank,msgi_local_RQ_meta);
    fprintf(stderr,"[%d]msgi_remote_RQ_meta: %ld\n",local_rank,msgi_remote_RQ_meta);

    fprintf(stderr,"[%d]msgi_local_RQe: %ld\n",local_rank,msgi_local_RQe);
    fprintf(stderr,"[%d]msgi_remote_RQe: %ld\n",local_rank,msgi_remote_RQe);
    fprintf(stderr,"[%d]###################################\n",local_rank);
*/
#else
    int i;
    int index=0;
    for(i=0;i<ENV_CORE_NUM;i++)
    {
        if(i == local_rank)
        {
            index++;
            continue;
        }
        else
        {
            // For the local request queues
            msgi_local_RQ_meta[i] = rq_meta_base;
            msgi_local_RQe[i] = rq_buffer_base;
            p = (msgi_RQ_meta_t *) (uintptr_t)msgi_local_RQ_meta[i];
            p->producer = 0;
            p->consumer = 0;

            rq_meta_base += msgi_rq_meta_size;
            rq_buffer_base += msgi_rq_buffer_size;

            // For the remote request queues
            if(index)
            {
                msgi_remote_RQ_meta[i] = shm_address_location + (i * (ENV_CORE_NUM -1) * sizeof(msgi_RQ_meta_t)) + local_rank * sizeof(msgi_RQ_meta_t);
                msgi_remote_RQe[i] = shm_address_location + (ENV_CORE_NUM * (ENV_CORE_NUM-1) * (sizeof(msgi_RQ_meta_t))) + (i * (ENV_CORE_NUM - 1) * msgi_rq_buffer_size) + local_rank * msgi_rq_buffer_size;

                p = (msgi_RQ_meta_t *) (uintptr_t)msgi_remote_RQ_meta[i];
                p->producer = 0;
                p->consumer = 0;
            }
            else
            {
                msgi_remote_RQ_meta[i] = shm_address_location + (i * (ENV_CORE_NUM -1) * sizeof(msgi_RQ_meta_t)) + (local_rank-1) * sizeof(msgi_RQ_meta_t);
                msgi_remote_RQe[i] = shm_address_location + (ENV_CORE_NUM * (ENV_CORE_NUM-1) * (sizeof(msgi_RQ_meta_t))) + (i * (ENV_CORE_NUM - 1) * msgi_rq_buffer_size) + (local_rank - 1) * msgi_rq_buffer_size;

                p = (msgi_RQ_meta_t *) (uintptr_t)msgi_remote_RQ_meta[i];
                p->producer = 0;
                p->consumer = 0;
            }
        }
    }
#endif
    /* 
       sleep(local_rank);
       fprintf(stderr,"[%d]###################################\n",local_rank);
       for(i=0;i<ENV_CORE_NUM;i++)
       {
       fprintf(stderr,"msgi_local_RQ_meta[%d]: %ld\n",i,msgi_local_RQ_meta[i]);
       fprintf(stderr,"msgi_remote_RQ_meta[%d]: %ld\n",i,msgi_remote_RQ_meta[i]);
       }
       for(i=0;i<ENV_CORE_NUM;i++)
       {
       fprintf(stderr,"msgi_local_RQe[%d]: %ld\n",i,msgi_local_RQe[i]);
       fprintf(stderr,"msgi_remote_RQe[%d]: %ld\n",i,msgi_remote_RQe[i]);
       }
       fprintf(stderr,"[%d]###################################\n",local_rank);
     */

    return MSG_SUCCESS;
}

MSG_ERR_T msgi_send_recv_exit(void *argc,void *argv)
{
    MSG_ERR_T rc = MSG_SUCCESS;

    return rc;
}
