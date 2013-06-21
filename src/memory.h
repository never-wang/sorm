/****************************************************************************
 *       Filename:  memory.h
 *
 *    Description:  j
 *
 *        Version:  1.0
 *        Created:  06/13/2013 02:04:17 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#ifndef MEMORY_H
#define MEMORY_H

/** @brief: allocator  */
typedef struct mem_allocator_s
{
    void*(*_alloc)(void *memory_pool, size_t size);
    void(*_free)(void *memory_pool, void *point);
    char*(*_strdup)(void *memory_pool, const char *string);

    void *memory_pool;
}mem_allocator_t;

void* mem_malloc(size_t size);
void mem_free(void *ptr);
char* mem_strdup(const char *str);

#endif
