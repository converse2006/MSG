/* --------------------------------------------------------------  */
/* (C)Copyright 2011                                               */
/* Po-Hsun, Chiu <maxshow0906@gmail.com>                           */
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

#ifndef _MSG_COLLECTIVE_H_
#define _MSG_COLLECTIVE_H_

#include "msg.h"
#include "msg_internal.h"

extern MSG_ERR_T msgi_bcast(void *buffer,int count,MSG_Datatype data_type,int root,MSG_Comm comm);

extern MSG_ERR_T msgi_reduce(void *buffer,void *result,int count,MSG_Datatype data_type,MSG_Op op,int root,MSG_Comm comm);

extern MSG_ERR_T msgi_barrier(MSG_Comm comm);

extern MSG_ERR_T msgi_gather(void *send_buffer,int send_count,MSG_Datatype send_type,void *recv_buffer,int recv_count,MSG_Datatype recv_type,int root,MSG_Comm comm);

extern MSG_ERR_T msgi_scatter(void *send_buffer,int send_count,MSG_Datatype send_type,void *recv_buffer,int recv_count,MSG_Datatype recv_type,int root,MSG_Comm comm);

#endif
