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

extern int local_rank;
extern int numpros;
/*-------------------------------*/

MSG_ERR_T msgi_reduce(void *buffer,void *result,int count,MSG_Datatype data_type,MSG_Op op,int root,MSG_Comm comm)
{
    MSG_ERR_T rc = MSG_SUCCESS;

    MSG_Status stat;
    int sum,max,min;
    int tmp;
    int data_type_sizeb=0;
    int sizeb=0;

    data_type_sizeb = check_data_type(data_type);
    sizeb = count*data_type_sizeb;

#ifdef REDUCE_OPT
    int generation=1;
    int power = 1;
    int two_to_generation;
    int step;
    int virtual_rank;

    while((power << 1) < numpros)
    {
        power = power << 1;
        generation++;
    }

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

    switch(op)
    {
        case MSG_SUM:
            if(!virtual_rank)
            {
                memcpy((void *)&sum,buffer,sizeb);

                for(step=generation-1;step>=0;step--)
                {
                    two_to_generation = POW_OF_2(step);
                    //????
                    if(virtual_rank >= two_to_generation && 
                            virtual_rank < (two_to_generation << 1))
                    {
                        printf("OH MY GOD!\n");
                        //printf("[%d]recv data from %d to %d\n",step,virtual_rank-two_to_generation,virtual_rank);
                        //msg_recv(buffer,count,data_type,virtual_rank-division_to_generation,step,comm,&stat); 
                    }
                    //receiver
                    else if(virtual_rank < two_to_generation &&
                            ((virtual_rank+two_to_generation) <= (numpros-1)))
                    {
                        //printf("[%d]recv data from %d to %d\n",step,virtual_rank+two_to_generation,virtual_rank);
                        msg_recv((void *)&tmp,count,data_type,virtual_rank+two_to_generation,step,comm,&stat);
                        sum += tmp; 
                    }

                }
                memcpy(result,(void *)&sum,sizeb);
            }
            else
            {
                memcpy((void *)&sum,buffer,sizeb);

                for(step=generation-1;step>=0;step--)
                {
                    two_to_generation = POW_OF_2(step);

                    //sender
                    if(virtual_rank >= two_to_generation && 
                            virtual_rank < (two_to_generation << 1))
                    {
                        //printf("[%d]send data from %d to %d\n",step,virtual_rank,virtual_rank-two_to_generation);
                        msg_send((void *)&sum,count,data_type,virtual_rank-two_to_generation,step,comm); 
                    }
                    //receiver
                    else if(virtual_rank < two_to_generation &&
                            ((virtual_rank+two_to_generation) <= (numpros-1)))
                    {

                        //printf("[%d]recv data from %d to %d\n",step,virtual_rank+two_to_generation,virtual_rank);
                        msg_recv((void *)&tmp,count,data_type,virtual_rank+two_to_generation,step,comm,&stat);
                        sum+=tmp;
                    }
                }
            }

            break;
        case MSG_MAX:
            if(!virtual_rank)
            {
                memcpy((void *)&max,buffer,sizeb);

                for(step=generation-1;step>=0;step--)
                {
                    two_to_generation = POW_OF_2(step);
                    //????
                    if(virtual_rank >= two_to_generation && 
                            virtual_rank < (two_to_generation << 1))
                    {
                        printf("OH MY GOD!\n");
                        //printf("[%d]recv data from %d to %d\n",step,virtual_rank-two_to_generation,virtual_rank);
                        //msg_recv(buffer,count,data_type,virtual_rank-division_to_generation,step,comm,&stat); 
                    }
                    //receiver
                    else if(virtual_rank < two_to_generation &&
                            ((virtual_rank+two_to_generation) <= (numpros-1)))
                    {
                        //printf("[%d]recv data from %d to %d\n",step,virtual_rank+two_to_generation,virtual_rank);
                        msg_recv((void *)&tmp,count,data_type,virtual_rank+two_to_generation,step,comm,&stat);
                        max = MAX(max,tmp);
                    }

                }
                memcpy(result,(void *)&max,sizeb);
            }
            else
            {
                memcpy((void *)&max,buffer,sizeb);

                for(step=generation-1;step>=0;step--)
                {
                    two_to_generation = POW_OF_2(step);

                    //sender
                    if(virtual_rank >= two_to_generation && 
                            virtual_rank < (two_to_generation << 1))
                    {
                        //printf("[%d]send data from %d to %d\n",step,virtual_rank,virtual_rank-two_to_generation);
                        msg_send((void *)&max,count,data_type,virtual_rank-two_to_generation,step,comm); 
                    }
                    //receiver
                    else if(virtual_rank < two_to_generation &&
                            ((virtual_rank+two_to_generation) <= (numpros-1)))
                    {

                        //printf("[%d]recv data from %d to %d\n",step,virtual_rank+two_to_generation,virtual_rank);
                        msg_recv((void *)&tmp,count,data_type,virtual_rank+two_to_generation,step,comm,&stat);
                        max = MAX(max,tmp);
                    }
                }
            }
            break;
        case MSG_MIN:
            if(!virtual_rank)
            {
                memcpy((void *)&min,buffer,sizeb);

                for(step=generation-1;step>=0;step--)
                {
                    two_to_generation = POW_OF_2(step);
                    //????
                    if(virtual_rank >= two_to_generation && 
                            virtual_rank < (two_to_generation << 1))
                    {
                        printf("OH MY GOD!\n");
                        //printf("[%d]recv data from %d to %d\n",step,virtual_rank-two_to_generation,virtual_rank);
                        //msg_recv(buffer,count,data_type,virtual_rank-division_to_generation,step,comm,&stat); 
                    }
                    //receiver
                    else if(virtual_rank < two_to_generation &&
                            ((virtual_rank+two_to_generation) <= (numpros-1)))
                    {
                        //printf("[%d]recv data from %d to %d\n",step,virtual_rank+two_to_generation,virtual_rank);
                        msg_recv((void *)&tmp,count,data_type,virtual_rank+two_to_generation,step,comm,&stat);
                        min = MIN(min,tmp);
                    }

                }
                memcpy(result,(void *)&min,sizeb);
            }
            else
            {
                memcpy((void *)&min,buffer,sizeb);

                for(step=generation-1;step>=0;step--)
                {
                    two_to_generation = POW_OF_2(step);

                    //sender
                    if(virtual_rank >= two_to_generation && 
                            virtual_rank < (two_to_generation << 1))
                    {
                        //printf("[%d]send data from %d to %d\n",step,virtual_rank,virtual_rank-two_to_generation);
                        msg_send((void *)&min,count,data_type,virtual_rank-two_to_generation,step,comm); 
                    }
                    //receiver
                    else if(virtual_rank < two_to_generation &&
                            ((virtual_rank+two_to_generation) <= (numpros-1)))
                    {

                        //printf("[%d]recv data from %d to %d\n",step,virtual_rank+two_to_generation,virtual_rank);
                        msg_recv((void *)&tmp,count,data_type,virtual_rank+two_to_generation,step,comm,&stat);
                        min = MIN(min,tmp);
                    }
                }
            }
            break;
        default:
            break;
    }

#else
    int i;


    switch(op)
    {
        case MSG_SUM:
            if(local_rank == root)
            {
                memcpy((void *)&sum,buffer,sizeb);
                //printf("sum: %d\n",sum);

                for(i=0;i<numpros;i++)
                {
                    if(i==root)
                        continue;
                    else
                    {
                        msg_recv((void *)&tmp,count,data_type,i,local_rank,comm,&stat);
                        sum += tmp;
                        //printf("[%d]sum: %d\n",i,sum);
                    }
                }
                memcpy(result,(void *)&sum,sizeb);
            }
            else
            {
                msg_send(buffer,count,data_type,root,root,comm);
            }
            break;
        case MSG_MAX:
            if(local_rank == root)
            {
                memcpy((void *)&max,buffer,sizeb);
                //printf("max: %d\n",max);

                for(i=0;i<numpros;i++)
                {
                    if(i==root)
                        continue;
                    else
                    {
                        msg_recv((void *)&tmp,count,data_type,i,local_rank,comm,&stat);
                        max = MAX(max,tmp);
                        //printf("[%d]max: %d\n",i,max);
                    }
                }
                memcpy(result,(void *)&max,sizeb);
            }
            else
            {
                msg_send(buffer,count,data_type,root,root,comm);
            }
            break;
        case MSG_MIN:
            if(local_rank == root)
            {
                memcpy((void *)&min,buffer,sizeb);
                //printf("min: %d\n",min);

                for(i=0;i<numpros;i++)
                {
                    if(i==root)
                        continue;
                    else
                    {
                        msg_recv((void *)&tmp,count,data_type,i,local_rank,comm,&stat);
                        min = MIN(min,tmp);
                        //printf("[%d]min: %d\n",i,min);
                    }
                }
                memcpy(result,(void *)&min,sizeb);
            }
            else
            {
                msg_send(buffer,count,data_type,root,root,comm);
            }
            break;
        default:
            break;
    }

#endif

    return rc;
}

//  =====   CONSTRUCTOR and DESTRUCTOR  =======
MSG_ERR_T msgi_reduce_init(void *argc,void *argv)
{
    MSG_ERR_T rc = MSG_SUCCESS;
    return rc;
}

MSG_ERR_T msgi_reduce_exit(void *argc,void *argv)
{
    MSG_ERR_T rc = MSG_SUCCESS;
    return rc;
}
