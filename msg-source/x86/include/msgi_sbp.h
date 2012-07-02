/*
 * Copyright (C) 2011 PASLab CSIE NTU. All rights reserved.
 *      - Chiu Po-Hsun <maxshow0906@gmail.com>
 */


#ifndef _MSGI_SBP_H_
#define _MSGI_SBP_H_

#include "msg.h"


#define BUSY                            100
#define FREE                            101
#define MSGI_MAX_SBP_BUFFERS            4
#define MSGI_MAX_SBP_BLOCKS_PER_BUFFER  4
#define MSGI_SBP_BLOCK_SIZE             4096    //block_size == 4K
#define MSGI_SBP_BUFFER_SIZE            MSGI_SBP_BLOCK_SIZE * MSGI_MAX_SBP_BLOCKS_PER_BUFFER

#define MSGI_NON_BLOCK_BUFFERS          256
#define MSGI_NON_BLOCK_BUFFER_SIZE      4096

#define GET_ADDR(base_addr, idx, b_size) (long *)(base_addr + (idx * b_size))

typedef struct msgi_sbp_meta
{
    volatile unsigned int producer;
    volatile unsigned int consumer;
    volatile unsigned int control_flag;
    unsigned int pad[5];
} msgi_sbp_meta_t;  //aligned 32

typedef struct msgi_non_block_buffer_meta
{
    volatile unsigned int control_flag;
    unsigned int pad[7];
} msgi_non_block_buffer_meta_t;  //aligned 32

/****************************************/
/* Inline Functions                     */
/****************************************/
static inline unsigned int count_blocks(unsigned int data_size, unsigned int block_size)
{
    unsigned int count = 0;

    if (((count = data_size / block_size) > 0) && ((data_size % block_size) == 0))
        return count;

    return count + 1;
}

#endif  // _MSGI_SBP_H_

