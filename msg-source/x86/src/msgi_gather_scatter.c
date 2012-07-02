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

extern char *shm_address;
extern long shm_address_location;
extern int local_rank;
extern int numpros;
/*-------------------------------*/

MSG_ERR_T msgi_gather(void *send_buffer,int send_count,MSG_Datatype send_type,void *recv_buffer,int recv_count,MSG_Datatype recv_type,int root,MSG_Comm comm)
{
    MSG_ERR_T rc = MSG_SUCCESS;
    MSG_Status stat;

#ifdef GATHER_OPT
    int i;
    int step=0;     //d
    int sender=0;   //i
    int block_size=1; //b
    int M=TREE_CHUNK_SIZE;
    int virtual_rank = local_rank;
    char *p=NULL;

    int tmp_numpros;
    if(numpros < M)
        tmp_numpros = numpros;
    else
        tmp_numpros = (int)(numpros/M) * M;

    //re-rank
    if(!root)
        virtual_rank = local_rank;
    else
    {
        if(local_rank == root)
            virtual_rank = 0;
        else if(local_rank > root)
            virtual_rank = local_rank - root;
        else
            virtual_rank = (tmp_numpros - root) + local_rank;
    }

    if((numpros < M) || (root < tmp_numpros))
    {
        if(local_rank == root)
        {
            p = recv_buffer;
            for(i=0;i<numpros;i++)
            {
                if(i == root)
                {
                    memcpy((void *)p,send_buffer,recv_count*sizeof(char));
                    //memcpy((void *)p,send_buffer,send_count*sizeof(char));
                }
                else
                {
                    rc = msg_recv((void *)p,recv_count,recv_type,i,i,MSG_COMM_WORLD,&stat);
                }
                p+=send_count*sizeof(char);
                //p+=recv_count*sizeof(char);
            }
        }
        else
        {
            rc = msg_send(send_buffer,recv_count,recv_type,root,local_rank,MSG_COMM_WORLD);
            //rc = msg_send(send_buffer,send_count,send_type,root,local_rank,MSG_COMM_WORLD);
        }

    }
    else
    {
        if(virtual_rank < tmp_numpros)
        {
            if(!virtual_rank)
            {
                memcpy(recv_buffer,send_buffer,recv_count*sizeof(char));

                p = recv_buffer+send_count*sizeof(char);

                //p = recv_buffer+recv_count*sizeof(char);
                //while(sender + POW_OF_2(step) < numpros)
                while(sender + POW_OF_2(step) < tmp_numpros)
                {
                    if((step == 0) || (sender >= M))
                    {
                        sender += POW_OF_2(step);
                        //printf("Step%d: sender: %d\n",step,sender);
                    }
                    else
                    {
                        sender += POW_OF_2(step-1);
                        //printf("QQStep%d: sender: %d\n",step,sender);
                    }

                    //if((sender + block_size) > numpros)
                    //    block_size = numpros - sender;
                    if((sender + block_size) > tmp_numpros)
                        block_size = tmp_numpros - sender;

                    rc = msg_recv(p,block_size*recv_count,recv_type,sender,sender,comm,&stat);
                    p += block_size*send_count*sizeof(char);
                    //p += block_size*recv_count*sizeof(char);
                    //printf("Step%d: root recv from %d, %d blocks\n",step,sender,block_size);

                    if((block_size << 1) <= M)
                    {
                        step += 1;
                        //block_size *= 2;
                        block_size = block_size << 1;
                        //printf("root step: %d, block_size:%d\n",step,block_size);
                    }
                }


                //handle the remain processes
                if(tmp_numpros < numpros)
                {  
                    //printf("HANDLE THE REMAIN PROCESSES\n");
                    p = recv_buffer+recv_count*sizeof(char)*tmp_numpros;
                    for(i=tmp_numpros;i<numpros;i++)
                    {
                        rc = msg_recv((void *)p,recv_count,recv_type,i,i,comm,&stat);
                        p += recv_count*sizeof(char);
                        //printf("ROOT RECV FROM %d\n",i);
                    }
                }
            }
            else
            {
                int tmp_b;
                char *communication_buffer;
                communication_buffer = (char *)malloc(send_count*sizeof(char)*M);

                //copy data from user buffer to communication buffer
                memcpy((void *)communication_buffer,send_buffer,recv_count*sizeof(char));
                //memcpy((void *)communication_buffer,send_buffer,send_count*sizeof(char));

                //p = communication_buffer+(recv_count*sizeof(char));
                p = communication_buffer+(send_count*sizeof(char));

                while((NBIT(virtual_rank,step) == 0) && ((block_size << 1) <= M))
                {
                    //if(virtual_rank+POW_OF_2(step) < numpros)
                    if(virtual_rank+POW_OF_2(step) < tmp_numpros)
                    {
                        //if(virtual_rank+POW_OF_2(step)+block_size > numpros)
                        //    tmp_b = numpros - virtual_rank - POW_OF_2(step);
                        if(virtual_rank+POW_OF_2(step)+block_size > tmp_numpros)
                            tmp_b = tmp_numpros - virtual_rank - POW_OF_2(step);
                        else
                            tmp_b = block_size;

                        msg_recv((void *)p,tmp_b*recv_count,recv_type,virtual_rank+POW_OF_2(step),virtual_rank+POW_OF_2(step),comm,&stat);
                        //printf("[%d]recv from %d, %d blocks\n",virtual_rank,virtual_rank+POW_OF_2(step),tmp_b);

                        p += tmp_b*send_count*sizeof(char); 
                        //p += tmp_b*recv_count*sizeof(char); 

                        block_size += tmp_b;
                    }
                    step += 1;
                }

                if((block_size << 1) > M)
                {
                    //msg_send((void *)communication_buffer,block_size*recv_count,recv_type,0,virtual_rank,comm);
                    msg_send((void *)communication_buffer,block_size*send_count,send_type,0,virtual_rank,comm);
                    //printf("[%d]send to 0, %d blocks\n",virtual_rank,block_size);
                }
                else
                {
                    //msg_send((void *)communication_buffer,block_size*recv_count,recv_type,virtual_rank-POW_OF_2(step),virtual_rank,comm);
                    msg_send((void *)communication_buffer,block_size*send_count,send_type,virtual_rank-POW_OF_2(step),virtual_rank,comm);
                    //printf("[%d]send to %d, %d blocks\n",virtual_rank,virtual_rank-POW_OF_2(step),block_size);
                }
            }
        }
        else
        {
            //printf("VIRTUAL RANK: %d SEND DATA TO ROOT\n",virtual_rank);
            rc = msg_send(send_buffer,recv_count,recv_type,0,virtual_rank,comm);
            //rc = msg_send(send_buffer,send_count,send_type,0,virtual_rank,comm);
        }
    }

#else
    int i;

    if(local_rank == root)
    {
        char *p = NULL;
        p = recv_buffer;
        for(i=0;i<numpros;i++)
        {
            if(i == root)
            {
                memcpy((void *)p,send_buffer,send_count*sizeof(char));
            }
            else
            {
                rc = msg_recv((void *)p,recv_count,recv_type,i,i,MSG_COMM_WORLD,&stat);
            }
            p+=recv_count*sizeof(char);
        }
    }
    else
    {
        rc = msg_send(send_buffer,send_count,send_type,root,local_rank,MSG_COMM_WORLD);
    }
#endif

    return rc;
}

MSG_ERR_T msgi_scatter(void *send_buffer,int send_count,MSG_Datatype send_type,void *recv_buffer,int recv_count,MSG_Datatype recv_type,int root,MSG_Comm comm)
{
    MSG_ERR_T rc = MSG_SUCCESS;
    MSG_Status stat;

#ifdef SCATTER_OPT1
    int generation=1;
    int power = 1;
    int two_to_generation;
    int step;
    //char *buffer;

    //buffer = malloc(send_count*sizeof(char)*numpros);

    while((power << 1) < numpros)
    {
        power = power << 1;
        generation++;
    }
    //printf("generation: %d\n",generation);

    if(!root)
    {
        //if(!local_rank)
        //    memcpy((void *)buffer,send_buffer,send_count*sizeof(char)*numpros);


        for(step=0;step<generation;step++)
        {
            //two_to_generation = (int)pow(2,step);
            two_to_generation = POW_OF_2(step);
            //receiver
            if(local_rank >= two_to_generation && 
                    local_rank < (two_to_generation << 1))
            {
                //printf("[%d]recv data from %d to %d\n",step,local_rank-two_to_generation,local_rank);
                //msg_recv(buffer,recv_count*numpros,recv_type,local_rank-two_to_generation,step,comm,&stat); 
                msg_recv(send_buffer,recv_count*numpros,recv_type,local_rank-two_to_generation,step,comm,&stat); 
            }
            //sender
            else if(local_rank < two_to_generation &&
                    ((local_rank+two_to_generation) <= (numpros-1)))
            {
                //printf("[%d]send data from %d to %d\n",step,local_rank,local_rank+two_to_generation);
                //msg_send(buffer,send_count*numpros,send_type,local_rank+two_to_generation,step,comm);
                msg_send(send_buffer,send_count*numpros,send_type,local_rank+two_to_generation,step,comm);
            }
        }
    }
    else
    {
        printf("YYY\n");
    }

    //copy data to its recv_buffer
    //memcpy(recv_buffer,(void *)(buffer+local_rank*recv_count),recv_count*sizeof(char));
    memcpy(recv_buffer,(void *)(send_buffer+local_rank*recv_count),recv_count*sizeof(char));

#elif SCATTER_OPT2
    int i;
    int step=0;     //d
    int receiver=0;   //i
    int block_size=1; //b
    int M=TREE_CHUNK_SIZE;
    int virtual_rank = local_rank;
    char *p=NULL;

    int tmp_numpros;
    if(numpros < M)
        tmp_numpros = numpros;
    else
        tmp_numpros = (int)(numpros/M) * M;

    if(numpros < M)
    {
        if(local_rank == root)
        {
            p = send_buffer;
            for(i=0;i<numpros;i++)
            {
                if(i == root)
                {
                    memcpy(recv_buffer,(void *)p,recv_count*sizeof(char));
                }
                else
                {
                    rc = msg_send((void *)p,send_count,send_type,i,i,MSG_COMM_WORLD);
                }
                p+=send_count*sizeof(char);
            }
        }
        else
        {
            rc = msg_recv(recv_buffer,recv_count,recv_type,root,local_rank,MSG_COMM_WORLD,&stat);
        }
    }
    else
    {
        if(virtual_rank < tmp_numpros)
        {
            if(!virtual_rank)
            {
                //copy its data
                memcpy(recv_buffer,send_buffer,recv_count*sizeof(char));

                p = send_buffer+send_count*sizeof(char);
                //while(sender + POW_OF_2(step) < numpros)
                while(receiver + POW_OF_2(step) < tmp_numpros)
                {
                    if((step == 0) || (receiver >= M))
                    {
                        receiver += POW_OF_2(step);
                        //printf("Step%d: receiver: %d\n",step,receiver);
                    }
                    else
                    {
                        receiver += POW_OF_2(step-1);
                        //printf("QQStep%d: receiver: %d\n",step,receiver);
                    }

                    //if((receiver + block_size) > numpros)
                    //    block_size = numpros - receiver;
                    if((receiver + block_size) > tmp_numpros)
                        block_size = tmp_numpros - receiver;

                    //rc = msg_recv(p,block_size*recv_count,recv_type,sender,sender,comm,&stat);
                    rc = msg_send(p,block_size*send_count,send_type,receiver,receiver,comm);
                    p += block_size*send_count*sizeof(char);
                    printf("Step%d: root send to %d, %d blocks\n",step,receiver,block_size);

                    if((block_size << 1) <= M)
                    {
                        step += 1;
                        //block_size *= 2;
                        block_size = block_size << 1;
                    }
                }

                //handle the remain processes
                if(tmp_numpros < numpros)
                {  
                    printf("HANDLE THE REMAIN PROCESSES\n");
                    p = send_buffer+send_count*sizeof(char)*tmp_numpros;
                    for(i=tmp_numpros;i<numpros;i++)
                    {
                        //rc = msg_recv((void *)p,recv_count,recv_type,i,i,comm,&stat);
                        rc = msg_send((void *)p,send_count,send_type,i,virtual_rank,comm);
                        p += send_count*sizeof(char);
                        printf("ROOT SEND TO %d\n",i);
                    }
                }
            }
            else
            {
                int tmp_b;
                char *communication_buffer;
                communication_buffer = (char *)malloc(recv_count*sizeof(char)*M);

                //copy data from user buffer to communication buffer
                //memcpy((void *)communication_buffer,send_buffer,send_count*sizeof(char));

                //p = communication_buffer+(send_count*sizeof(char));

                //cal the block_size to identify where to recv
                while((NBIT(virtual_rank,step) == 0) && ((block_size << 1) <= M))
                {
                    if(virtual_rank+POW_OF_2(step) < tmp_numpros)
                    {
                        if(virtual_rank+POW_OF_2(step)+block_size > tmp_numpros)
                            tmp_b = tmp_numpros - virtual_rank - POW_OF_2(step);
                        else
                            tmp_b = block_size;

                        //msg_recv((void *)p,tmp_b*recv_count,recv_type,virtual_rank+POW_OF_2(step),virtual_rank+POW_OF_2(step),comm,&stat);
                        //printf("[%d]recv from %d, %d blocks\n",virtual_rank,virtual_rank+POW_OF_2(step),tmp_b);

                        //p += tmp_b*recv_count*sizeof(char); 

                        block_size += tmp_b;
                    }
                    step += 1;
                }

                //printf("[%d]step: %d, block_size: %d\n",virtual_rank,step,block_size);

                if((block_size << 1) > M)
                {
                    msg_recv((void *)communication_buffer,block_size*recv_count,recv_type,0,0,comm,&stat);
                    printf("[%d]recv from 0, %d blocks\n",virtual_rank,block_size);
                }
                else
                {
                    msg_recv((void *)communication_buffer,block_size*recv_count,recv_type,virtual_rank-POW_OF_2(step),virtual_rank-POW_OF_2(step),comm,&stat);
                    printf("[%d]recv from %d, %d blocks\n",virtual_rank,virtual_rank-POW_OF_2(step),block_size);
                }

                //init the variables
                step = 0;
                block_size = 1;

                memcpy(recv_buffer,(void *)communication_buffer,recv_count*sizeof(char));
                p = communication_buffer+(recv_count*sizeof(char));

                //scatter to other slave
                while((NBIT(virtual_rank,step) == 0) && ((block_size << 1) <= M))
                {
                    //if(virtual_rank+POW_OF_2(step) < numpros)
                    if(virtual_rank+POW_OF_2(step) < tmp_numpros)
                    {
                        //if(virtual_rank+POW_OF_2(step)+block_size > numpros)
                        //    tmp_b = numpros - virtual_rank - POW_OF_2(step);
                        if(virtual_rank+POW_OF_2(step)+block_size > tmp_numpros)
                            tmp_b = tmp_numpros - virtual_rank - POW_OF_2(step);
                        else
                            tmp_b = block_size;

                        //msg_recv((void *)p,tmp_b*recv_count,recv_type,virtual_rank+POW_OF_2(step),virtual_rank+POW_OF_2(step),comm,&stat);
                        msg_send((void *)p,tmp_b*send_count,send_type,virtual_rank+POW_OF_2(step),virtual_rank,comm);
                        printf("[%d]send to %d, %d blocks\n",virtual_rank,virtual_rank+POW_OF_2(step),tmp_b);

                        p += tmp_b*send_count*sizeof(char); 

                        block_size += tmp_b;
                    }
                    step += 1;
                }
                //memcpy(recv_buffer,(void *)communication_buffer,recv_count*sizeof(char));
            }
        }
        else
        {
            //printf("VIRTUAL RANK: %d RECV DATA FROM ROOT\n",virtual_rank);
            //rc = msg_send(send_buffer,send_count,send_type,0,virtual_rank,comm);
            rc = msg_recv(recv_buffer,recv_count,recv_type,0,0,comm,&stat);
        }
    }

#else
    int i;

    if(local_rank == root)
    {
        char *p = NULL;
        p = send_buffer;
        for(i=0;i<numpros;i++)
        {
            if(i == root)
            {
                memcpy(recv_buffer,(void *)p,recv_count*sizeof(char));
            }
            else
            {
                rc = msg_send((void *)p,send_count,send_type,i,i,comm);
            }
            p+=send_count*sizeof(char);
        }
    }
    else
    {
        rc = msg_recv(recv_buffer,recv_count,recv_type,root,local_rank,comm,&stat);
    }
#endif
    return rc;
}
