/*
 * Copyright (C) 2011 PASLab CSIE NTU. All rights reserved.
 *      - Chiu Po-Hsun <maxshow0906@gmail.com>
 */

#include <sys/wait.h>

#include "msg.h"
#include "msgi_q.h"
#include "msgi_sbp.h"

extern long shm_address_location;


#define x86_SHM_KEY                 1234
#define x86_NON_BLOCK_SHM_KEY       1235
#define x86_SERVICE_PROCESSOR_SHM_KEY   1236

//rq_meta + rq_data + sbp_meta + sbp_data
#define SHM_SIZE    (ENV_CORE_NUM *(ENV_CORE_NUM-1)*32)+(ENV_CORE_NUM *(ENV_CORE_NUM-1)*MSGI_MAX_RQE*MSGI_MAX_RQE_SIZE) + (ENV_CORE_NUM*MSGI_MAX_SBP_BUFFERS*32) + (ENV_CORE_NUM*MSGI_MAX_SBP_BLOCKS_PER_BUFFER*MSGI_MAX_SBP_BUFFERS*MSGI_SBP_BLOCK_SIZE)

//rq_meta + rq + buffer_meta + buffers
#define NON_BLOCK_SHM_SIZE  (ENV_CORE_NUM * 32) + (ENV_CORE_NUM * MSGI_NON_BLOCK_RQE * MSGI_NON_BLOCK_RQE_SIZE) + (MSGI_NON_BLOCK_BUFFERS * 32) (MSGI_NON_BLOCK_BUFFERS * MSGI_NON_BLOCK_BUFFER_SIZE) 

//rq_meta + rq + rfq_meta + rfq + sbp_meta + sbp_data
#define SERVICE_PROCESSOR_SHM_SIZE  (ENV_CORE_NUM * 32) + (ENV_CORE_NUM * MSGI_MAX_RQE * MSGI_MAX_RQE_SIZE) + (ENV_CORE_NUM * 32) + (ENV_CORE_NUM * MSGI_MAX_RQE * MSGI_MAX_RQE_SIZE) + (ENV_CORE_NUM*MSGI_MAX_SBP_BUFFERS*32) + (ENV_CORE_NUM*MSGI_MAX_SBP_BLOCKS_PER_BUFFER*MSGI_MAX_SBP_BUFFERS*MSGI_SBP_BLOCK_SIZE)

//#define SHM_SIZE    1057152 //default 1MB

/*
enum MEMORY_MAP
{
    // Request Queue
    // Meta-data
    MSGI_RQ_META_SIZE               =   sizeof(msgi_RQ_meta_t),
    MSGI_RQ_START                   =   shm_address_location,

    // Data Buffer
    MSGI_RQ_BUFFER_SIZE             =   (MSGI_MAX_RQE_SIZE * MSGI_MAX_RQE),
    MSGU_RQ_BUFFER_START            =   (MSGI_RQ_START + (ENV_CORE_NUM * (ENV_CORE_NUM - 1) * MSGI_RQ_META_SIZE)),

    // Send Buffer Pool 
    // Meta-data
    MSGI_SBP_META_SIZE              =   (32 * MSGI_MAX_SBP_BUFFERS),
    MSGI_SBP_META_START             =   (MSGI_RQ_BUFFER_START + (ENV_CORE_NUM * (ENV_CORE_NUM - 1) * MSGI_RQ_BUFFER_SIZE)),
    
    // Data Buffer
    MSGI_SBP_POOL_SIZE              =   16384,
    MSGI_SBP_BUFFER_START           =   MSGI_SBP_META_START + (MSGI_SBP_META_SIZE * ENV_CORE_NUM),
};
*/
