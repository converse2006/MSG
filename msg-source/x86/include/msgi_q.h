/*
 * Copyright (C) 2011 PASLab CSIE NTU. All rights reserved.
 *      - Chiu Po-Hsun <maxshow0906@gmail.com>
 */

#ifndef _MSGI_Q_H_
#define _MSGI_Q_H_

#include "msg.h"

#define MSGI_MAX_RQE        4
#define MSGI_MAX_RQE_SIZE   128

#define MSGI_NON_BLCOK_RQE      4
#define MSGI_NON_BLOCK_RQE_SIZE 32

typedef struct msgi_RQe
{
    unsigned int  index;        // 0x00: Index of buffer in SBP
    unsigned int  len;          // 0x04: Message length
    unsigned int  tag;          // 0x08: Message tag
    msg_addr_32_t status_addr;  // 0x0c: 
    unsigned int  sid;          // 0x10: sender ID
    unsigned int  rid;          // 0x14: receiver ID
    unsigned int  pad[2];
} msgi_RQe_t;

typedef struct msgi_RQ_meta
{
    volatile unsigned int producer;
    volatile unsigned int consumer;
    volatile unsigned int pad[6];
} msgi_RQ_meta_t; // aligned 32

typedef struct msgi_non_block_RQe
{
    unsigned int  index;        // 0x00: Index of buffer in SBP
    unsigned int  len;          // 0x04: Message length
    unsigned int  tag;          // 0x08: Message tag
    msg_addr_32_t status_addr;  // 0x0c: 
    unsigned int  sid;          // 0x10: sender ID
    unsigned int  rid;          // 0x14: receiver ID
    unsigned int  pad[2];
} msgi_non_block_RQe_t;

typedef struct msgi_non_block_RQ_meta
{
    volatile unsigned int total_in;
    volatile unsigned int total_out;
    volatile unsigned int pad[6];
} msgi_non_block_RQ_meta_t; // aligned 32

#endif  // _MSGI_Q_H_
