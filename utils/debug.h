/****************************************************************************
 *       Filename:  debug.h
 *
 *    Description:  define for debug mask
 *
 *        Version:  1.0
 *        Created:  12/28/2011 03:24:20 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#ifndef _DEBUG_H
#define _DEBUG_H

#include <stdio.h>
#include <stdint.h>

#define ENABLE_DEBUG

#define DEBUG_NONE  ((uint32_t)0)
#define DEBUG_USER  ((uint32_t)1 << 0)
#define DEBUG_IOFW  ((uint32_t)1 << 1)
#define DEBUG_PACK  ((uint32_t)1 << 2)
#define DEBUG_TIME  ((uint32_t)1 << 3)
#define DEBUG_MSG   ((uint32_t)1 << 4)
#define DEBUG_UNMAP ((uint32_t)1 << 4)

extern int debug_mask;

#ifdef ENABLE_DEBUG
/* try to avoid function call overhead by checking masks in macro */
#define debug(mask, format, f...)                  \
    if (debug_mask & mask)                         \
    {                                                     \
        printf("[%s, %s, %d]: " format "\n", __FILE__ , __func__, __LINE__ , ##f); \
    }                                                     
#else
#define debug(mask, format, f...) \
    do {} while(0)
#endif

#define set_debug_mask(mask) debug_mask = mask
#define add_debug_mask(mask) debug_mask |= mask

#define debug_mark(mask) debug(mask, "MARK")

#define error(format, f...) \
    printf("[%s, %s, %d]: "format "\n", __FILE__, __func__, __LINE__, ##f);

#endif
