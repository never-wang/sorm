#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <assert.h>

#include "debug.h"
#include "times.h"

time_stack_t *time_stack;

double times_cur()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return (double)(time.tv_sec * 1000.0 + time.tv_usec / 1000.0);
}

int times_init()
{
    time_stack = malloc(sizeof(time_stack_t));
    if(NULL == time_stack)
    {
	error("malloc fail");
	return -1;
    }

    time_stack->size = TIMES_STACK_INIT_SIZE;
    time_stack->top = -1;
    time_stack->data = malloc(sizeof(double) * TIMES_STACK_INIT_SIZE);
    if(NULL == time_stack->data)
    {
	error("malloc fail");
	return -1;
    }

    return 0;
}

int times_final()
{
    if(NULL != time_stack)
    {
	if(NULL != time_stack->data)
	{
	    free(time_stack->data);
	}
	free(time_stack);
	time_stack = NULL;
    }

    return 0;
}

int times_start()
{
    double cur_time;
    void *addr;

    assert(NULL != time_stack);

    cur_time = times_cur();

    if(time_stack->top == time_stack->size - 1)
    {
	addr = realloc(time_stack->data, 
		time_stack->size * 2 * sizeof(double));
	if(NULL == time_stack->data)
	{
	    error("realloc fail");
	    return -1;
	}
	time_stack->size *= 2;
	time_stack->data = addr;
    }

    time_stack->top ++;
    time_stack->data[time_stack->top] = cur_time;

    return 0;
}

double times_end()
{
    double cur_time, start_time;
    
    assert(NULL != time_stack);

    cur_time = times_cur();
    start_time = time_stack->data[time_stack->top];
    time_stack->top --;

    return (cur_time - start_time);
}
