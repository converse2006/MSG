/*
 * Copyright (C) 2011 PASLab CSIE NTU. All rights reserved.
 *      - Chiu Po-Hsun <maxshow0906@gmail.com>
 */

#ifndef _MSG_DEBUG_H_
#define _MSG_DEBUG_H_

//#include "msg_config.h"

#if defined(MSG_DEBUG)

#include <stdio.h>
#include <assert.h>

#define MSG_DEBUG_LEVEL 2

#define MSG_DBG(level, CATEGORY, str, ...) \
    MSG_DEBUG_##CATEGORY(level, str, ##__VA_ARGS__)

#define MSG_DEBUG_MESSAGE(level, str, ...) \
    if (level <= MSG_DEBUG_LEVEL) \
        fprintf(stderr, "+[MSG DBG] %s:%.3d: " str "\n", __FILE__, __LINE__, ##__VA_ARGS__); 

#define MSG_DEBUG_GENERAL(level, str, ...) \
    if (level <= MSG_DEBUG_LEVEL) \
        fprintf(stderr, "+[MSG DBG] " str "\n", ##__VA_ARGS__);

#define MSG_DEBUG_ASSERT(level, str, ...) \
    if (level <= MSG_DEBUG_LEVEL) \
        assert((str));
#else

#define MSG_DBG(level, CATEGORY, str, ...)
#define MSG_DEBUG_MESSAGE(level, str, ...)
#define MSG_DEBUG_GENERAL(level, str, ...)
#define MSG_DEBUG_ASSERT(level, str, ...)

#endif

#endif // _MSG_DEBUG_H_
