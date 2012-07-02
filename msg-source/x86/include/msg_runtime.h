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

#ifndef _MSG_RUNTIME_H_
#define _MSG_RUNTIME_H_

#include "msg.h"

extern MSG_ERR_T msgi_init(int argc,char **argv);

extern MSG_ERR_T msgi_exit();

extern MSG_ERR_T msgi_comm_rank(MSG_Comm comm,int *rank);

extern MSG_ERR_T msgi_comm_size(MSG_Comm comm,int *size);

extern unsigned long long msgi_wtime();

#endif  //_MSG_RUNTIME_H_
