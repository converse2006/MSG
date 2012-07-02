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

#ifndef _MSG_H_
#define _MSG_H_

/*--------------------------------------------------------------*/
/*	Includes                                                    */
/*--------------------------------------------------------------*/
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#ifdef MSG_x86
/*--------------------------------------------------------------*/
/*	System V IPC shared memory                                  */
/*--------------------------------------------------------------*/
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

/*--------------------------------------------------------------*/
/*	Debug macro                                                 */
/*--------------------------------------------------------------*/

#undef DBG
#ifdef DEBUG
#include <stdio.h>

#define DBG(str,...) do{ \
    fprintf(stderr, "[%s:%.3d] " str, __FILE__, __LINE__, ##__VA_ARGS__); \
    fprintf(stderr, "\n"); \
}while(0)
#else 
#define DBG(str,...)
#endif

#endif
/*--------------------------------------------------------------*/
/*	User Types                                                  */
/*--------------------------------------------------------------*/

//8-bit address
typedef char            msg_addr_8_t;

//32-bit address
typedef unsigned int    msg_addr_32_t;

typedef unsigned int    msg_lock_t;

typedef unsigned int    msg_wid_t;

// Representation of a thread, a process or a core
typedef unsigned int    node_id_t;

// Representation of a task runnig on a node
typedef unsigned int    msg_task_id_t;

// Representation of a process runnig on a node
typedef unsigned int    msg_process_id_t;

// MSG error code convention:
// - Some MSG "error" codes are actually status returned from API's.  These
// will be 0 or positive numbers.
// - All MSG error codes will start with MSG_ERR prefix.
// - All MSG codes indicating true errors will be negative numbers.
typedef enum
{
    MSG_SUCCESS                        =        0,  
    MSG_WID_READY                      =        1,  
    MSG_WID_BUSY                       =        2,  
    MSG_STS_PROC_RUNNING               =        3,  
    MSG_STS_PROC_FINISHED              =        4,  
    MSG_STS_PROC_FAILED                =        5,  
    MSG_STS_PROC_ABORTED               =        6,  
    MSG_STS_PROC_KILLED                =        7,  
    MSG_TRANS_NOT_READY                =        8,
    MSG_LAST_STATUS                    =        9,  

    //  errcodes should start at -35000 and count UP from there.
    //  therefore MSG_FIRST_ERROR will be the most negative #
    MSG_ERR_FIRST_ERROR                =   -35000, //< first errorcode
    MSG_ERR_INTERNAL                   =   -34999,
    MSG_ERR_SYSTEM                     =   -34998,
    MSG_ERR_INVALID_ARGV               =   -34997,
    MSG_ERR_INVALID_ENV                =   -34996,
    MSG_ERR_INVALID_HANDLE             =   -34995,
    MSG_ERR_INVALID_ADDR               =   -34994,
    MSG_ERR_INVALID_ATTR               =   -34993,
    MSG_ERR_INVALID_NODE               =   -34992,
    MSG_ERR_INVALID_PID                =   -34991,
    MSG_ERR_INVALID_TARGET             =   -34990,
    MSG_ERR_BUF_OVERFLOW               =   -34989,
    MSG_ERR_NOT_ALIGNED                =   -34988,
    MSG_ERR_INVALID_SIZE               =   -34987,
    MSG_ERR_BYTESWAP_MISMATCH          =   -34986,
    MSG_ERR_NO_RESOURCE                =   -34985,
    MSG_ERR_PROC_LIMIT                 =   -34984,
    MSG_ERR_NO_PERM                    =   -34983,
    MSG_ERR_OWNER                      =   -34982,
    MSG_ERR_NOT_OWNER                  =   -34981,
    MSG_ERR_RESOURCE_BUSY              =   -34980,
    MSG_ERR_GROUP_CLOSED               =   -34979,
    MSG_ERR_GROUP_OPEN                 =   -34978,
    MSG_ERR_GROUP_DUPLICATE            =   -34977,
    MSG_ERR_INVALID_WID                =   -34976,
    MSG_ERR_INVALID_STREAM             =   -34975,
    MSG_ERR_NO_WIDS                    =   -34974,
    MSG_ERR_WID_ACTIVE                 =   -34973,
    MSG_ERR_WID_NOT_ACTIVE             =   -34972,
    MSG_ERR_INITIALIZED                =   -34971,
    MSG_ERR_NOT_INITIALIZED            =   -34970,
    MSG_ERR_MUTEX_BUSY                 =   -34969,
    MSG_ERR_NOT_SUPPORTED_YET          =   -34968,
    MSG_ERR_VERSION_MISMATCH           =   -34967,
    MSG_ERR_MSGD_FAILURE               =   -34966,
    MSG_ERR_INVALID_PROG               =   -34965,
    MSG_ERR_ARCH_MISMATCH              =   -34964,
    MSG_ERR_INVALID_USERNAME           =   -34963,
    MSG_ERR_INVALID_CWD      		   =   -34962,
    MSG_ERR_NOT_FOUND           	   =   -34961,
    MSG_ERR_TOO_LONG            	   =   -34960,
    MSG_ERR_NODE_TERM           	   =   -34959,
    MSG_ERR_DATA_TYPE                  =   -34958,
    MSG_ERR_AFFINITY                   =   -34957,

    // TEMP
    MSG_ERR_OPEN                       =   -34958,
    MSG_ERR_DMEM                       =   -34957,
    MSG_ERR_MMAP                       =   -34956,
    MSG_ERR_BUF_EMPTY                  =   -34955,
    MSG_ERR_DMA                        =   -34954,

    // Add new error codes above here
    MSG_ERR_LAST_ERROR

} MSG_ERR_T;



// Types of NODEs in the topology
typedef enum MSG_NODE_TYPE
{	
    MSG_NODE_INVALID	= 0,	// INVALID TYPE
    MSG_NODE_SYSTEMX,			// Optern - supervises 4 Cell Blades	
    MSG_NODE_CELL_BLADE,		// Cell Blade - contains 2 CBE's
    MSG_NODE_CBE,				// Cell Broadband Engine
    MSG_NODE_SPE,				// Cell SPE
    MSG_NODE_ACC,				// Accelerator node
    MSG_NODE_SMP,               // SMP x86 node
    MSG_NODE_MAX_TYPE			// Always last - max NODE type
} MSG_NODE_TYPE_T;

/*
// Flags for msg_node_kill
typedef enum MSG_KILL_TYPE
{
MSG_KILL_TYPE_ASYNC
} MSG_KILL_TYPE_T;


// Implementation-specific flag that influences process creation.
//
//      MSG_PROC_LOCAL_FILE -  the local file format:  
//                              A fully-qualified POSIX-compliant pathname of 
//                              the executable file that can be accessed from 
//                              the local file system.  
//      MSG_PROC_LOCAL_FILE_LIST - the local file list format: 
//                              A fully-qualified POSIX-compliant pathname of a 
//                              text file that can be accessed from the local 
//                              file system.  This file contains a (text) list 
//                              of the executable file and all dependant 
//                              libraries. The file and all the libraries is 
//                              loaded onto the accelerator and executed. 
//                              The executable file must be the first thing in 
//                              the list.  
//      MSG_PROC_REMOTE_FILE - the remote file format: 
//                              A fully-qualified POSIX-compliant pathname of 
//                              the executable file that can be accessed from 
//                              the remote accelerator.  
//      MSG_PROC_EMBEDDED is the embedded image format: 
//                              A pointer to an executable image that is 
//                              embedded in the executable image. 
typedef enum MSG_PROC_CREATION_FLAG_T
{
MSG_PROC_LOCAL_FILE,
MSG_PROC_EMBEDDED,
} MSG_PROC_CREATION_FLAG_T;

typedef struct msg_program_handle {
unsigned int data[8];
} msg_program_handle_t;
 */


/*------------------------------------------------------------*/
/* 	Constants												  */
/*------------------------------------------------------------*/ 	

/*
#define MSGI_MAX_ERROR_STRINGS	50

// Constant used as a process_id to refer to all running processes on a node
#define MSG_PROCESS_ALL (0xFFFFFFFFFFFFFFFFULL)
*/

// Invalid rank (node id)
#define NULL_RANK 0
// Number of processes allowed to run on a NODE
#define MAX_PROCESSES_PER_NODE 8
/*
#define MSG_NODE_SELF -1U
#define MSG_PID_SELF -1ULL

// Number of DSPs per PACDuo
//#define NUM_ACCS_PER_PROCESSOR 2

// This allows sending to indirect streams
enum {
MSG_STREAM_ALL = 0xffffffff,
MSG_STREAM_UB  = 0xffffff00
};



// Refers to the parent rank in the topology.
#define MSG_NODE_PARENT -2U

// Refers to the parent process in the topology.
#define MSG_PID_PARENT  -2ULL

// Refers to the number of NODEs that a MPU can talk to directly
#define MAX_MPU_PEERS 2
 */

#define ENV_CORE_NUM    4


/*Communicators */
typedef int MSG_Comm;
#define MSG_COMM_WORLD  91
#define MSG_COMM_SELF   92

/*Reduce operation */
typedef int MSG_Op;
#define MSG_MAX         93
#define MSG_MIN         94
#define MSG_SUM         95

/* Data types
 * A more aggressive yet homogeneous implementation might want to 
 * make the values here the number of bytes in the basic type, with
 * a simple test against a max limit (e.g., 16 for long double), and
 * non-contiguous structures with indices greater than that.
 * 
 * Note: Configure knows these values for providing the Fortran optional
 * types.  Any changes here must be matched by changes
 * in configure.in
 */
/* IN MSG Version 0.1, we only support Data_type with MSG_CHAR */
typedef int MSG_Datatype;
#define MSG_CHAR           ((MSG_Datatype)1) 
#define MSG_UNSIGNED_CHAR  ((MSG_Datatype)2)
#define MSG_BYTE           ((MSG_Datatype)3)
#define MSG_SHORT          ((MSG_Datatype)4)
#define MSG_UNSIGNED_SHORT ((MSG_Datatype)5)
#define MSG_INT            ((MSG_Datatype)6)
#define MSG_UNSIGNED       ((MSG_Datatype)7)
#define MSG_LONG           ((MSG_Datatype)8)
#define MSG_UNSIGNED_LONG  ((MSG_Datatype)9)
#define MSG_FLOAT          ((MSG_Datatype)10)
#define MSG_DOUBLE         ((MSG_Datatype)11)
#define MSG_LONG_DOUBLE    ((MSG_Datatype)12)
#define MSG_LONG_LONG_INT  ((MSG_Datatype)13)
#define MSG_LONG_LONG      ((MSG_Datatype)13)
#define MSG_PACKED         ((MSG_Datatype)14)
#define MSG_LB             ((MSG_Datatype)15)
#define MSG_UB             ((MSG_Datatype)16)

/*MSG Status definition */
#define MSG_STATUS_SIZE 4
typedef struct {
    int count;
    int MSG_SOURCE;
    int MSG_TAG;
    int MSG_ERROR;
    //#if (MSG_STATUS_SIZE > 4)
    //    int extra[MSG_STATUS_SIZE - 4];
} MSG_Status;


/*------------------------------------------------------------*/
/* 	Functions Prototypes									  */
/*------------------------------------------------------------*/ 	

/* environment setup functions */
extern MSG_ERR_T msg_init(int argc, char **argv);

extern MSG_ERR_T msg_exit();

extern MSG_ERR_T msg_comm_rank(MSG_Comm,int *rank);

extern MSG_ERR_T msg_comm_size(MSG_Comm,int *size);

extern unsigned long long msg_wtime();

/* point-to-point communication functions */
extern MSG_ERR_T msg_send(void *src_data,int count,MSG_Datatype data_type,int dst_rank,int tag,MSG_Comm comm);

extern MSG_ERR_T msg_recv(void *dst_data,int count,MSG_Datatype data_type,int src_rank,int tag,MSG_Comm comm,MSG_Status *status);

extern MSG_ERR_T msg_bcast(void *buffer,int count,MSG_Datatype data_type,int root,MSG_Comm comm);

extern MSG_ERR_T msg_reduce(void *buffer,void *result,int count,MSG_Datatype data_type,MSG_Op op,int root,MSG_Comm comm);

extern MSG_ERR_T msg_barrier(MSG_Comm comm);

extern MSG_ERR_T msg_gather(void *send_buffer,int send_count,MSG_Datatype send_type,void *recv_buffer,int recv_count,MSG_Datatype recv_type,int root,MSG_Comm comm);

extern MSG_ERR_T msg_scatter(void *send_buffer,int send_count,MSG_Datatype send_type,void *recv_buffer,int recv_count,MSG_Datatype recv_type,int root,MSG_Comm comm);

#endif  // _MSG_H_
