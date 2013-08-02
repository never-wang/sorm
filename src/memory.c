/****************************************************************************
 *       Filename:  memory.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  06/13/2013 01:52:52 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#include <stdlib.h>
#include <string.h>

#include "memory.h"

/**
 * @brief: call system malloc
 */
static inline void* system_alloc(void *memory_pool, size_t size)
{
    void *buf;

    if(size == 0)
    {
        return NULL;
    }

    buf = malloc(size);
    if(buf == NULL)
    {
        error("Not enough memory.");
        exit(-1);
    }
    return buf;
}

/**
 * @brief: call system free
 */
static inline void system_free(void *memory_pool, void *pointer)
{
    if(pointer != NULL)
    {
        free(pointer);
    }
}

/**
 * @brief: call system strdup
 */
static inline char* system_strdup(void *memory_pool, const char *string)
{
    if(string != NULL)
    {
        return strdup(string);
    }
    return NULL;
}
    
static mem_allocator_t usr_allocator = 
{
    system_alloc,
    system_free,
    system_strdup,
    NULL,
};

static const mem_allocator_t sys_allocator = 
{
    system_alloc,
    system_free,
    system_strdup,
    NULL,
};

void mem_set_allocator(void *memory_pool, 
    void*(*_alloc)(void *memory_pool, size_t size), 
    void(*_free)(void *memory_pool, void *point), 
    char*(*_strdup)(void *memory_pool, const char *string))
{
    usr_allocator.memory_pool = memory_pool;
    usr_allocator._alloc = (_alloc == NULL) ? system_alloc : _alloc;
    usr_allocator._free = _free;
    usr_allocator._strdup = (_strdup == NULL) ? system_strdup : _strdup;
}

void* usr_malloc(size_t size)
{
    return usr_allocator._alloc(usr_allocator.memory_pool, size);
}

void usr_free(void *ptr)
{
    return usr_allocator._free(usr_allocator.memory_pool, ptr);
}

char* usr_strdup(const char *str)
{
    return usr_allocator._strdup(usr_allocator.memory_pool, str);
}

void* sys_malloc(size_t size)
{
    return sys_allocator._alloc(sys_allocator.memory_pool, size);
}

void sys_free(void *ptr)
{
    return sys_allocator._free(sys_allocator.memory_pool, ptr);
}

char* sys_strdup(const char *str)
{
    return sys_allocator._strdup(sys_allocator.memory_pool, str);
}
