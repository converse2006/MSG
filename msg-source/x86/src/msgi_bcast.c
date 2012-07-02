/* --------------------------------------------------------------  */
/* (C)Copyright 2011                                               */
/* Po-Hsun Chiu <maxshow0906@gmail.com>                            */
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
#include "msg_collective.h"
#include "msgi_q.h"
#include "msgi_sbp.h"
/*-------------------------------*/

int shmid_bcast;
volatile char *bcast_address;
volatile long bcast_flag_start[2];
volatile long bcast_relay_start[2];

extern int local_rank;
extern int numpros;
/*-------------------------------*/

/******static function****/
static inline int count_times(int data_size,int buffer_size);

MSG_ERR_T msgi_bcast(void *buffer,int count,MSG_Datatype data_type,int root,MSG_Comm comm)
{
    MSG_ERR_T rc = MSG_SUCCESS;

#ifdef BCAST_OPT1   //shared memory scheme
    int i,j;
    int times;
    int remain_size = count;
    int index = 0;
    volatile char *flag_ptr=NULL;

    times = count_times(count,BCAST_RELAY_SIZE);
    //    fprintf(stderr,"times: %d\n",times);

    if(local_rank == root)
    {
        for(i=0;i<times;i++)
        {
            index = i%2;
            flag_ptr = (char *)bcast_flag_start[index];
            //check flag for coping data to buffer
            for(j=0;j<numpros;j++)
            {
                if(*(flag_ptr+(j*sizeof(char))) != BCAST_INIT)
                {
                    j-=1;
                }
                else
                    continue;
            }

            //printf("root check\n");

            if(remain_size > BCAST_RELAY_SIZE)
                memcpy((void *)bcast_relay_start[index],buffer+i*BCAST_RELAY_SIZE,BCAST_RELAY_SIZE);
            else
                memcpy((void *)bcast_relay_start[index],buffer+i*BCAST_RELAY_SIZE,remain_size);

            remain_size -= BCAST_RELAY_SIZE;

            //printf("root copy\n");

            //set ready to indicate other processes to get data
            for(j=0;j<numpros;j++)
            {
                if(j==root)
                    continue;
                else
                    *(flag_ptr + j*sizeof(char)) = BCAST_SET;
            }
        }
    }
    else
    {
        for(i=0;i<times;i++)
        {
            index = i%2;
            flag_ptr = (char *)bcast_flag_start[index];
            //Each slave process checks the flag to ready
            while(*(flag_ptr + local_rank*sizeof(char)) != BCAST_SET){}

            if(remain_size > BCAST_RELAY_SIZE)
                memcpy(buffer+i*BCAST_RELAY_SIZE,(void *)bcast_relay_start[index],BCAST_RELAY_SIZE);
            else
                memcpy(buffer+i*BCAST_RELAY_SIZE,(void *)bcast_relay_start[index],remain_size);

            remain_size -= BCAST_RELAY_SIZE;

            //set the flag to init for indicating the root
            *(flag_ptr + local_rank*sizeof(char)) = BCAST_INIT;
        }
    }
#elif BCAST_OPT2    //binary tree
    int generation=1;
    int power = 1;
    int two_to_generation;
    int step;
    int virtual_rank;
    MSG_Status stat;

    while((power << 1) < numpros)
    {
        power = power << 1;
        generation++;
    }
    //printf("generation: %d\n",generation);

    //re-rank
    if(!root)
        virtual_rank = local_rank;
    else
    {
        if(local_rank < root)
            virtual_rank = local_rank + 1;
        else if(local_rank == root)
            virtual_rank = 0;
        else
            virtual_rank = local_rank;
    }
   
//    printf("local_rank: %d, virtual_rank: %d\n",local_rank,virtual_rank);

    //if(!root)
    if(!virtual_rank)
    {
        for(step=0;step<generation;step++)
        {
            //two_to_generation = (int)pow(2,step);
            two_to_generation = POW_OF_2(step);

            //receiver
            if(virtual_rank >= two_to_generation && 
                    virtual_rank < (two_to_generation << 1))
            {
                //printf("[%d]recv data from %d to %d\n",step,virtual_rank-two_to_generation,virtual_rank);
                msg_recv(buffer,count,data_type,virtual_rank-two_to_generation,step,comm,&stat); 
            }
            //sender
            else if(virtual_rank < two_to_generation &&
                    ((virtual_rank+two_to_generation) <= (numpros-1)))
            {
                //printf("[%d]send data from %d to %d\n",step,virtual_rank,virtual_rank+two_to_generation);
                msg_send(buffer,count,data_type,virtual_rank+two_to_generation,step,comm);
            }
        }
        /*
        for(step=0;step<generation;step++)
        {
            //two_to_generation = (int)pow(2,step);
            two_to_generation = SHIFT(step);
            //receiver
            if(local_rank >= two_to_generation && 
                    local_rank < (two_to_generation << 1))
            {
                //printf("[%d]recv data from %d to %d\n",step,local_rank-two_to_generation,local_rank);
                msg_recv(buffer,count,data_type,local_rank-two_to_generation,step,comm,&stat); 
            }
            //sender
            else if(local_rank < two_to_generation &&
                    ((local_rank+two_to_generation) <= (numpros-1)))
            {
                //printf("[%d]send data from %d to %d\n",step,local_rank,local_rank+two_to_generation);
                msg_send(buffer,count,data_type,local_rank+two_to_generation,step,comm);
            }
        }
        */
    }
    else
    {
        /*
        if(!local_rank)
            msg_recv(buffer,count,data_type,root,root,comm,&stat);
        else if(local_rank == root)
            msg_send(buffer,count,data_type,0,local_rank,comm);
        */
        for(step=0;step<generation;step++)
        {
            //two_to_generation = (int)pow(2,step);
            two_to_generation = POW_OF_2(step);
            //receiver
            if(virtual_rank >= two_to_generation && 
                    virtual_rank < (two_to_generation << 1))
            {
                printf("[%d]recv data from %d to %d\n",step,virtual_rank-two_to_generation,virtual_rank);
                msg_recv(buffer,count,data_type,virtual_rank-two_to_generation,step,comm,&stat); 
            }
            //sender
            else if(virtual_rank < two_to_generation &&
                    ((virtual_rank+two_to_generation) <= (numpros-1)))
            {
                //printf("[%d]send data from %d to %d\n",step,virtual_rank,virtual_rank+two_to_generation);
                msg_send(buffer,count,data_type,virtual_rank+two_to_generation,step,comm);
            }
        }

        /*
        for(step=0;step<generation;step++)
        {
            //two_to_generation = (int)pow(2,step);
            two_to_generation = SHIFT(step);
            //receiver
            if(local_rank >= two_to_generation && 
                    local_rank < (two_to_generation << 1))
            {
                //printf("[%d]recv data from %d to %d\n",step,local_rank-two_to_generation,local_rank);
                msg_recv(buffer,count,data_type,local_rank-two_to_generation,step,comm,&stat); 
            }
            //sender
            else if(local_rank < two_to_generation &&
                    ((local_rank+two_to_generation) <= (numpros-1)))
            {
                //printf("[%d]send data from %d to %d\n",step,local_rank,local_rank+two_to_generation);
                msg_send(buffer,count,data_type,local_rank+two_to_generation,step,comm);
            }
        }
        */

    }

#else
    int i;

    if(local_rank == root)
    {
        for(i=0;i<numpros;i++)
        {
            if(i == root)
                continue;
            else
            {
                msg_send(buffer,count,data_type,i,i,comm);
            }
        }
    }
    else
    {
        MSG_Status stat;
        msg_recv(buffer,count,data_type,root,local_rank,comm,&stat);
    }
#endif

    return rc;
}

//  =====   CONSTRUCTOR and DESTRUCTOR  =======
MSG_ERR_T msgi_bcast_init(void *argc,void *argv)
{
    MSG_ERR_T rc = MSG_SUCCESS;

#ifdef BCAST_OPT1
    key_t key = BCAST_KEY;
    int i;

    // get the shared memory id
    //double buffering
    if((shmid_bcast = shmget(key,(sizeof(char)*ENV_CORE_NUM)*2+BCAST_RELAY_SIZE*2,IPC_CREAT | 0666)) < 0)
    {perror("shmget for shmid_bcast failed");   exit(-1);}

    // Attach the segment to data space
    if((bcast_address = shmat(shmid_bcast,NULL,0)) == (char *) -1)
    {perror("shmat bcast_address failed");    exit(-1);}

    for(i=0;i<ENV_CORE_NUM*2;i++)
        *(bcast_address+(i*sizeof(char))) = BCAST_INIT;

    for(i=0;i<2;i++)
    {
        bcast_flag_start[i] = (long)(bcast_address + (i * sizeof(char)*ENV_CORE_NUM));
        bcast_relay_start[i] = (long)((bcast_address + (sizeof(char)*ENV_CORE_NUM*2)) + (i * BCAST_RELAY_SIZE));
    }

#elif BCAST_OPT2
#else
#endif

    return rc;
}

MSG_ERR_T msgi_bcast_exit(void *argc,void *argv)
{
    MSG_ERR_T rc = MSG_SUCCESS;

#ifdef BCAST_OPT1
    int errno;
    struct shmid_ds *shmid_ds=NULL;

    // Detach the shared memory
    if((errno = shmdt((void *)bcast_address)) == -1)
    {perror("shmdt bcast_address failed");    exit(-1);}


    // Remove the shared memory
    if((errno = shmctl(shmid_bcast,IPC_RMID,shmid_ds)) == -1)
    {perror("shmctl barrier_address failed");    exit(-1);}
    printf("Remove the bcast_address shared region\n");
#elif BCAST_OPT2
#else
#endif
    return rc;
}

/****************************************/
/* Inline Functions                     */
/****************************************/
static inline int count_times(int data_size,int buffer_size)
{
    int times=0;

    if(((times = data_size / buffer_size) > 0) && ((data_size % buffer_size) == 0))
        return times;

    return times+1;
}

