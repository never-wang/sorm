#ifndef TIMES_H_
#define TIMES_H_

#include <time.h>
#include <sys/time.h>

#define TIMES_STACK_INIT_SIZE 512

typedef struct time_stack_s
{
    double *data;
    int top;
    int size;
}time_stack_t;

/**
 * @brief: get current time, in ms
 *
 * @return: current time, in ms
 */
double times_cur();
/**
 * @brief: init time stack
 *
 * @return: 0 if success, <0 if fail
 */
int times_init();
/**
 * @brief: free the space of time stack
 *
 * @return: 0 if success
 */
int times_final();
/**
 * @brief: insert a start_time in time stack
 *
 * @return: 0 if success, <0 if fail
 */
int times_start();

/**
 * @brief: set a end_time tag, compute the time
 *
 * @return: time between start tag and end tag
 */
double times_end();

#endif
