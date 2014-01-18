/****************************************************************************
 *       Filename:  memory.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  06/13/2013 02:04:17 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *          Email:  never.wencan@gmail.com
 ***************************************************************************/
#ifndef MEMORY_H
#define MEMORY_H
#include <stdlib.h>
#include <string.h>

#include "sorm.h"

static inline void* _malloc(
        const sorm_allocator_t *allocator, size_t size) {
    void *buf;

    if(size == 0) {
        return NULL;
    }

    if (allocator == NULL) {
        buf = malloc(size);
    } else {
        buf = allocator->_alloc(allocator->memory_pool, size);
    }
    if(buf == NULL) {
        error("Not enough memory.");
        exit(-1);
    }
    return buf;
}

static inline void _free(
        const sorm_allocator_t *allocator, void *ptr) {
    if (allocator == NULL) {
        free(ptr);
    } else {
        allocator->_free(allocator->memory_pool, ptr);
    }
}

static inline char *_strdup(
        const sorm_allocator_t *allocator, const char *str) {
    char *buf;

    if (str == NULL) {
        return NULL;
    }

    if (allocator == NULL) {
        buf = strdup(str);
    }else {
        buf = allocator->_strdup(allocator->memory_pool, str);
    }
    if (buf == NULL) {
        error("Not enough memory.");
        exit(-1);
    }
}

#endif
