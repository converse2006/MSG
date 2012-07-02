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


#ifndef _MSG_INTERNAL_H_
#define _MSG_INTERNAL_H_


#define POW_OF_2(x) (1 << (x))
#define NBIT(var,x) ((var) & POW_OF_2(x))
#define MAX(a,b)    (((a) > (b)) ? (a): (b))
#define MIN(a,b)    (((a) < (b)) ? (a): (b))

#include "msg.h"
#include "msg_debug.h"

// ==== "CONSTRUCTOR" CALLS
extern MSG_ERR_T msgi_topology_init(void *,void *);
extern MSG_ERR_T msgi_send_recv_init(void *argc,void *argv);
extern MSG_ERR_T msgi_mm_init(void *,void *);
extern MSG_ERR_T msgi_sbp_init();
extern MSG_ERR_T msgi_process_init(int *);
extern MSG_ERR_T msgi_barrier_init(void *,void *);
extern MSG_ERR_T msgi_bcast_init(void *,void *);
extern MSG_ERR_T msgi_reduce_init(void *,void *);
extern MSG_ERR_T msgi_service_processor_init(void *,void *);

// ==== "DESTRUCTOR" CALLS
extern MSG_ERR_T msgi_topology_exit(void);
extern MSG_ERR_T msgi_send_recv_exit(void *argc,void *argv);
extern MSG_ERR_T msgi_mm_exit(void *,void *);
extern MSG_ERR_T msgi_sbp_exit();
extern MSG_ERR_T msgi_process_exit();
extern MSG_ERR_T msgi_barrier_exit(void *,void *);
extern MSG_ERR_T msgi_bcast_exit(void *,void *);
extern MSG_ERR_T msgi_reduce_exit(void *,void *);
extern MSG_ERR_T msgi_service_processor_exit(void *,void *);

// ==== Sender Buffer Pool====
extern MSG_ERR_T msgi_copy_from_sbp(int src_rank,int buf_idx, void *data,int sizeb);
extern MSG_ERR_T msgi_copy_to_sbp(int buf_idx, void *data,int sizeb);

// Signature to verify correctness of the MSG Control Block
#define MSG_SIGNATURE 0xdeadbeef

// shared memory key number
#define BARRIER_KEY     100
#define BCAST_KEY       101

//flag for BARRIER
#define BARRIER_INIT    0
#define BARRIER_SET     1

//flag for BCAST
#define BCAST_RELAY_SIZE    4096
#define BCAST_INIT    0
#define BCAST_SET     1

//flag for GATHER and SCATTER
#define TREE_CHUNK_SIZE 8

//control macro for small-large scheme
#define SMALL       102
#define LARGE       103

// Today we only support the use of the first PIDS slot in the topology
// structure.  This define is used for this single index, but it also provides
// an easy way to locate all the code that would need to be adjusted if
// multiple pids/tids were supported.
#define ONLYPID 0               


/****************************************/
/* Inline Functions                     */
/****************************************/
static inline int check_data_type(MSG_Datatype data_type)
{
    switch(data_type)
    {
        case MSG_CHAR:
            return sizeof(char);
            break;
        case MSG_UNSIGNED_CHAR:
             return sizeof(unsigned char);
            break;
        case MSG_BYTE:
            return sizeof(char);
            break;
        case MSG_SHORT:
            return sizeof(short);
            break;
        case MSG_UNSIGNED_SHORT:
            return sizeof(unsigned short);
            break;
        case MSG_INT:
            return sizeof(int);
            break;
        case MSG_UNSIGNED:
            return sizeof(unsigned);
            break;
        case MSG_LONG:
            return sizeof(long);
            break;
        case MSG_UNSIGNED_LONG:
            return sizeof(unsigned long);
            break;
        case MSG_FLOAT:
            return sizeof(float);
            break;
        case MSG_DOUBLE:
            return sizeof(double);
            break;
        case MSG_LONG_DOUBLE:
            return sizeof(long double);
            break;
        case MSG_LONG_LONG:
            return sizeof(long long);
            break;
        default:
            break;
    }
    return 0;
}


typedef enum msgi_msg_type 
{
    MSGI_MSG_RECV = 0,
    MSGI_MSG_SEND = 1,
    MSGI_NUM_MSG_TYPE
} msgi_msg_type_t;

typedef enum msgi_dmrQ_reg
{
    MSGI_DMRQ_UNREGISTERED,
    MSGI_DMRQ_REGISTERED
} msgi_dmrQ_reg_t;

typedef struct msgi_cb
{
    union
    {   
        struct
        {   
            uint32_t            signature;                      //  0: 'MSG'
            uint32_t            index;                          //  4: Index of this node
            node_id_t           rank;                           //  8: id for the node
            MSG_NODE_TYPE_T     type;                           // 12: type of node
            msg_process_id_t    tid;                            // 16: id for task running on the node 
            msg_addr_32_t       remote_msgQ[MSGI_NUM_MSG_TYPE]; // 24: MS addr of the remote msg Qs    
            msg_addr_32_t       msgQ_lock[MSGI_NUM_MSG_TYPE];   // 40: bytes: msg Qs lock info.
            msg_addr_32_t       errstrlist;                     // 56: mainstore location of the list of error strings.
            MSG_NODE_TYPE_T     ptype;                          // 64: parent tyep
            unsigned int        put_align;                      // 68: DMA put alignment to use.
        };
        unsigned char           pad[128];                       // pad to 128 bytes
    };
} msgi_cb_t;

extern msgi_cb_t msg_cb[ENV_CORE_NUM];

//
// msg_process_id_t will point to the address of a variable that contains
// a pid or a pthread_t type
//
typedef struct msg_ptid
{
    union {
#if defined(MSG_MPU)
        pthread_t           pthread_id;                     // thread id
#else   // Accelerator - no pthread_t in acc include files
        uint32_t            pthread_id;                     // thread id
#endif
        //pid_t             pid_id;                         // process id
        uint32_t            pid_id;                         // process id
    };
} msg_ptid_t;

//
// Process ID status
// 
typedef enum MSGI_PID_STS
{
    MSGI_PID_INVALID    =   'I',        // 'I' pid status is invalid
    MSGI_PID_RUNNING    =   'R',        // 'R' process is running
    MSGI_PID_STOPPED    =   'S',        // 'S' process has stopped
    MSGI_PID_TERM       =   'T',        // 'T' process finished or died
} MSGI_PID_STS_T;

//
// status of the rank represented by this topology struct
//
typedef enum MSGI_NODE_STS
{
    MSGI_NODE_INVALID   =   0x47,       //< 'G' node status is invalid
    MSGI_NODE_FREE,                     //< 'H' node is free
    MSGI_NODE_RESERVED                  //< 'I' node is reserved
} MSGI_NODE_STS_T;

//
// Descriptor of NODE process termination information
//
typedef struct msg_stop_info
{   
    unsigned int stop_reason;
    union {
        int exit_code;
        int signal_code;
        int runtime_error;
        int runtime_exception;
        int runtime_fatal;
        int callback_error;
        void *__reserved_ptr;
        unsigned long long __reserved_u64;
    } result;
    int acc_status;
} msg_stop_info_t;                          


//
// Descriptor of each process running on a node
//
typedef struct msg_node_pid
{
    msg_ptid_t          pid;            // process/thread id
    void                *prog_handle;   // program handle
    MSGI_PID_STS_T      status;         // status
    unsigned int        flags;          // thread flags
    unsigned int        entry;          // thread PC
    msg_stop_info_t     stopinfo;       // context termination info
    unsigned int        nwaiters;       // number of waiters present
    msg_addr_32_t       argv;           // 4 bytes: argv pointer
    msg_addr_32_t       envv;           // 4 bytes: envv pointer 
} msg_node_pid_t;


//
// Topology tree descriptor. The topology is considered to be linked to
// the hardware structure, thus the pointer to the runtime control block.
//
typedef struct msg_topology
{
    union
    {
        struct
        {
            MSGI_NODE_STS_T         reservation;        // reserved flag
            uint32_t                my_index;           // my index in the parent's children array
            msg_addr_32_t           ctx;                // "handle" of the node context: speid, pthreadid, etc.
            msgi_cb_t               *cb;                // pointer to control block of the current node.
            struct msg_topology     *parent;            // ptr ot parent topology node
            uint32_t                num_peers;          // number of peers
            struct msg_topology     *peers;             // ptr to arrary of peer topo's
            uint32_t                num_children;       // number of children
            struct msg_topology     *children;          // ptr to array of children topo's
            uint16_t                nproc_supported;    
            uint16_t                nproc_active;
            msg_node_pid_t          pids[MAX_PROCESSES_PER_NODE];
            uint32_t                dma_lock;           // DMA synchronization
            uint32_t                dmrQ_reg;
            msg_addr_32_t           dsm_area_phy;           
            msg_addr_32_t           dsm_area_vir;           
            msg_addr_32_t           ldm_area_phy;           
            msg_addr_32_t           ldm_area_vir;
            msg_addr_32_t           sram_area_phy;
            msg_addr_32_t           sram_area_vir;
        };
        unsigned char pad[128];
    };
} msg_topology_t;





#endif
