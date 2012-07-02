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
/*-------------------------------*/

/*MSG header---------------------*/
#include "msg.h"
#include "msg_collective.h"
/*-------------------------------*/
int shmid_barrier;
volatile char *barrier_address;

extern int local_rank;
extern int numpros;
/*-------------------------------*/


MSG_ERR_T msgi_barrier(MSG_Comm comm)
{
    MSG_ERR_T rc = MSG_SUCCESS;

#ifdef BARRIER_OPT
    //test
    if(!local_rank)
    {
        int i;
        for(i=1;i<numpros;i++)
        {
            if(*(barrier_address+(i*sizeof(int))) != BARRIER_SET)
            {
                i-=1;
            }
            else
            {
                continue;
            }
        }
        for(i=1;i<numpros;i++)
            *(barrier_address+(i*sizeof(int))) = BARRIER_INIT;
    }
    else
    {
        *(barrier_address+(local_rank*sizeof(int))) = BARRIER_SET;

        while(*(barrier_address+(local_rank*sizeof(int))) != BARRIER_INIT){}
    }

#else
    if(!local_rank)
    {
        usleep(1);
        *barrier_address = 0;
    }

    while(*barrier_address != local_rank){}

    *barrier_address += 1;
    //printf("[%d]numpros: %d,add address: %d\n",local_rank,numpros,*barrier_address);

    while(*barrier_address != numpros){}

    //printf("[%d]leave\n",local_rank);
#endif

    return rc;
}

//  =====   CONSTRUCTOR and DESTRUCTOR  =======
MSG_ERR_T msgi_barrier_init(void *argc,void *argv)
{
    MSG_ERR_T rc = MSG_SUCCESS;

    key_t key = BARRIER_KEY;

#ifdef BARRIER_OPT
    int i;

    // get the shared memory id
    if((shmid_barrier = shmget(key,sizeof(int)*ENV_CORE_NUM,IPC_CREAT | 0666)) < 0)
    {perror("shmget for shnid_barrier failed");   exit(-1);}
    
    // Attach the segment to data space
    if((barrier_address = shmat(shmid_barrier,NULL,0)) == (char *) -1)
    {perror("shmat barrier_address failed");    exit(-1);}

    for(i=0;i<ENV_CORE_NUM;i++)
        *(barrier_address+(i*sizeof(int))) = BARRIER_INIT;

#else

    // get the shared memory id
    if((shmid_barrier = shmget(key,sizeof(int),IPC_CREAT | 0666)) < 0)
    {perror("shmget for shmid_barrier failed");   exit(-1);}

    // Attach the segment to data space
    if((barrier_address = shmat(shmid_barrier,NULL,0)) == (char *) -1)
    {perror("shmat barrier_address failed");    exit(-1);}

    //init the shared address
    *barrier_address = 0;
#endif

    return rc;
}

MSG_ERR_T msgi_barrier_exit(void *argc,void *argv)
{
    MSG_ERR_T rc = MSG_SUCCESS;
    int errno;
    struct shmid_ds *shmid_ds=NULL;

    // Detach the shared memory
    if((errno = shmdt((void *)barrier_address)) == -1)
    {perror("shmdt barrier_address failed");    exit(-1);}


    // Remove the shared memory
    if((errno = shmctl(shmid_barrier,IPC_RMID,shmid_ds)) == -1)
    {perror("shmctl barrier_address failed");    exit(-1);}
    printf("Remove the barrier_address shared region\n");

    return rc;
}
