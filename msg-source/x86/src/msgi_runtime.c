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
#include "msg_runtime.h"
/*-------------------------------*/

#ifdef SET_AFFINITY
/*Set core affinity--------------*/
#define __USE_GNU
#include <sched.h>
#include <ctype.h>
/*-------------------------------*/
#endif

/* global variables--------------*/
node_id_t msgi_local_rank = NULL_RANK;
msgi_cb_t msg_cb[ENV_CORE_NUM];
int numpros;
int shmid;
char *shm_address;
long shm_address_location;
int non_block_shmid;
char *non_block_shm_address;
long non_block_shm_address_location;
#ifdef SERVICE_PROCESSOR
int service_processor_shmid;
char *service_processor_shm_address;
long service_processor_shm_address_location;
#endif
//volatile char *barrier_address;
int local_rank;
//flag showing that msg_runtime_init() has been run
volatile int msgi_runtime_initialized = 0;
/*-------------------------------*/

/* RQ and SBP init--------------*/
//RQ meta-data
int msgi_rq_meta_size;
long msgi_rq_meta_start;
//RQ Data Buffer
int msgi_rq_buffer_size;
long msgi_rq_buffer_start;

#ifdef SERVICE_PROCESSOR
long msgi_rfq_meta_start;
long msgi_rfq_buffer_start;
#endif

int buffer_size;
int block_size;
//Send Buffer Pool meta-data
int msgi_sbp_meta_size;
long msgi_sbp_meta_start;
//Send Buffer Pool Data Buffer
int msgi_sbp_pool_size;
long msgi_sbp_buffer_start;
/*-------------------------------*/

/* local prototypes--------------*/
static void set_cb_entry(int, node_id_t,MSG_NODE_TYPE_T,MSG_NODE_TYPE_T);
static inline MSG_ERR_T set_core_affinity(void *argc,void *argv);
/*-------------------------------*/

MSG_ERR_T msgi_init(int argc,char **argv)
{
    int i                   =   0;
    MSG_ERR_T rc            =   MSG_SUCCESS;
    MSG_NODE_TYPE_T my_type =   MSG_NODE_INVALID;


    /* set the number of processes*/
    numpros = atoi(argv[2]);
    if(numpros <= 0)
    {perror("set numpros failed"); exit(-1);}
    
    //msgi_cb_t   msg_cb[numpros];
    // Fill in the processes control blocks
    for(i=0;i<numpros;i++)
    {
        set_cb_entry(i,msgi_local_rank+i,my_type,0);
    }

    rc = msgi_topology_init(NULL,NULL);
    if(rc != MSG_SUCCESS)
    return rc;

    //set up parent memory map
    if(rc == MSG_SUCCESS)
        rc = msgi_mm_init(NULL,NULL);

    if(rc == MSG_SUCCESS)
        rc = msgi_barrier_init(NULL,NULL);

    if(rc == MSG_SUCCESS)
        rc = msgi_bcast_init(NULL,NULL);

#ifdef SERVICE_PROCESSOR
    if(rc == MSG_SUCCESS)
        rc = msgi_service_processor_init(NULL,NULL);
#endif

    // processes control
    if(rc == MSG_SUCCESS)
        rc = msgi_process_init(&local_rank);
    //printf("[%d]runtime PID: %d\n",local_rank,(int)getpid());
    
#ifdef SET_AFFINITY
    if(rc == MSG_SUCCESS)
        rc = set_core_affinity(NULL,NULL);
#endif

    //set up sender buffer pool
    if(rc == MSG_SUCCESS)
    rc = msgi_sbp_init();

    //set up messaging (RQ information)
    if(rc == MSG_SUCCESS)
    rc = msgi_send_recv_init(NULL,NULL);

    //set runtime flag to true
    if(rc == MSG_SUCCESS)
        msgi_runtime_initialized = 1;

    //barrier
    rc = msg_barrier(MSG_COMM_WORLD);

    return rc;
}

MSG_ERR_T msgi_exit()
{
    MSG_ERR_T rc = MSG_SUCCESS;

    if(!msgi_runtime_initialized)
        return MSG_ERR_NOT_INITIALIZED;

#ifdef SERVICE_PROCESSOR
    if(!local_rank)
    {
        if(rc == MSG_SUCCESS)
            rc = msgi_service_processor_exit(NULL,NULL);
    }
#endif

    //detach memory map
    if(rc == MSG_SUCCESS)
        rc = msgi_mm_exit(NULL,NULL);

    //detach the barrier_address memory
    if(rc == MSG_SUCCESS)
        rc = msgi_barrier_exit(NULL,NULL);

    //detach the bcast_address memory
    if(rc == MSG_SUCCESS)
        rc = msgi_bcast_exit(NULL,NULL);

    //finalize sender buffer pool
    if(rc == MSG_SUCCESS)
        rc = msgi_sbp_exit();

    //finalize messaging (RQ information)
    if(rc == MSG_SUCCESS)
        rc = msgi_send_recv_exit(NULL,NULL);

    //printf("end of msg_exit\n");
    return rc;
}

MSG_ERR_T msgi_comm_rank(MSG_Comm comm,int *rank)
{
    *rank = local_rank;

    return MSG_SUCCESS;
}

MSG_ERR_T msgi_comm_size(MSG_Comm comm,int *size)
{
    *size = numpros;

    return MSG_SUCCESS;
}

//  =====   INTERNAL FUNCTIONS  =======
static void set_cb_entry(int index, node_id_t rank, MSG_NODE_TYPE_T type, 
        MSG_NODE_TYPE_T ptype)
{
    msg_cb[index].signature    =   MSG_SIGNATURE;
    msg_cb[index].index        =   index;
    msg_cb[index].rank         =   rank;
    msg_cb[index].type         =   type;
    msg_cb[index].ptype        =   ptype;  
}

#ifdef SET_AFFINITY
//Exploiting core affinity
static inline MSG_ERR_T set_core_affinity(void *argc,void *argv)
{
    cpu_set_t mask;
//    cpu_set_t get;

    CPU_ZERO(&mask);
    CPU_SET(local_rank%ENV_CORE_NUM,&mask);
    
    if(sched_setaffinity(0,sizeof(mask),&mask) == -1)
    {
        printf("warning: could not set CPU affinity, continuing...\n");
        return MSG_ERR_AFFINITY;
    }

    return MSG_SUCCESS;
}
#endif
