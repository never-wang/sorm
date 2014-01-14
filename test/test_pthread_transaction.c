/****************************************************************************
 *       Filename:  test_pthread_transaction.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  01/14/2014 01:14:55 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  
 ***************************************************************************/
#include <assert.h>
#include <pthread.h>
#include <stdio.h>

#include "sorm.h"
#include "device_sorm.h"

#define DB_FILE "pthread.db"

pthread_cond_t cond_a = PTHREAD_COND_INITIALIZER;
int condition_a = 0, condition_b = 0;
pthread_cond_t cond_b = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_rwlock_t rwlock;

static void* pthread_a_work(void* args) {
    sorm_connection_t *_conn;
    int ret = sorm_open(DB_FILE, SORM_DB_SQLITE, 0, &rwlock, 
            SORM_ENABLE_RWLOCK, &_conn);
    assert(ret == SORM_OK);
    
    pthread_mutex_lock(&mutex);
    while (condition_a == 0) {
        pthread_cond_wait(&cond_a, &mutex); 
    }
    pthread_mutex_unlock(&mutex);
    
    sorm_begin_write_transaction(_conn);
    
    device_t *device;
    ret = device_select_by_uuid(_conn, ALL_COLUMNS, "uuid0",
            &device);
    assert(ret == SORM_OK);
    printf("get small num : %d\n", device->small_num);
    
    sorm_commit_transaction(_conn);

    printf("a finish\n");

    sorm_close(_conn);
}

static void* pthread_b_work(void* args) {
    sorm_connection_t *_conn;
    int ret = sorm_open(DB_FILE, SORM_DB_SQLITE, 0, &rwlock, 
            SORM_ENABLE_RWLOCK, &_conn);
    assert(ret == SORM_OK);
    
    sorm_begin_read_transaction(_conn);
    
    pthread_mutex_lock(&mutex);
    condition_a = 1;
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&cond_a);

    printf("wait for input\n");
    ret = getchar();
    printf("continue running\n");
    
    device_t *device;
    ret = device_select_by_uuid(_conn, ALL_COLUMNS, "uuid0",
            &device);
    assert(ret == SORM_OK);
    printf("get small num : %d\n", device->small_num);
    device_free(device);
    
    sorm_commit_transaction(_conn);

    printf("b finish\n");
    sorm_close(_conn);
}

int main() {
    pthread_rwlock_init(&rwlock, NULL);
    pthread_t p1, p2;
    
    sorm_init(0);
    sorm_connection_t *_conn;
    int ret = sorm_open(DB_FILE, SORM_DB_SQLITE, 0, &rwlock, 
            SORM_ENABLE_RWLOCK, &_conn);
    ret = device_create_table(_conn);
    assert(ret == SORM_OK);
    device_t *device = device_new();
    device_set_uuid(device, "uuid0");
    device_set_small_num(device, 1);
    ret = device_save(_conn, device);
    assert(ret == SORM_OK);
    device_free(device);
    
    sorm_close(_conn);
    
    ret = pthread_create(&p1, NULL, pthread_a_work, (void*)_conn);
    assert(ret == 0);
    ret = pthread_create(&p2, NULL, pthread_b_work, (void*)_conn);
    assert(ret == 0);
    
    pthread_join(p1, NULL);
    pthread_join(p2, NULL);
}
