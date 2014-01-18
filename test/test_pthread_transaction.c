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

    device_t *device;
    sorm_iterator_t *iterator;
    
    ret = device_select_iterate_by_open(_conn, ALL_COLUMNS,
            NULL, &iterator);
    
    sorm_begin_read_transaction(_conn);
    int i;
    for (i = 0; i < 4; i ++) {
        device_select_iterate_by(iterator, &device);
        char string[1024];
        printf("select : %s\n", 
                device_to_string(device, string, 1024));
        device_free(device);
    }
    sorm_begin_write_transaction(_conn);
    device = device_new();
    device_set_id(device, 3);
    device_set_uuid(device, "uuid-3");
    device_set_name(device, "name-3");
    device_set_password(device, "passwd-3");
    ret = device_save(_conn, device);
    assert(ret == SORM_OK);
    device_free(device);
    printf("save uuid-3\n");
    pthread_mutex_lock(&mutex);
    condition_b = 1;
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&cond_b);
    
    printf("wait for input\n");
    ret = getchar();
    printf("continue running\n");
    
    sorm_commit_transaction(_conn);
    
    while (1) {
        device_select_iterate_by(iterator, &device);
        if (!device_select_iterate_more(iterator)) {
            break;
        }
        char string[1024];
        printf("select : %s\n", 
                device_to_string(device, string, 1024));
        device_free(device);
    }
    sorm_commit_transaction(_conn);
    device_select_iterate_close(iterator);
    
    //sorm_commit_transaction(_conn);

    printf("a finish\n");

    sorm_close(_conn);
}

static void* pthread_b_work(void* args) {
    sorm_connection_t *_conn;
    int ret = sorm_open(DB_FILE, SORM_DB_SQLITE, 0, &rwlock, 
            SORM_ENABLE_RWLOCK, &_conn);
    assert(ret == SORM_OK);
    
    pthread_mutex_lock(&mutex);
    while (condition_b == 0) {
        pthread_cond_wait(&cond_b, &mutex); 
    }
    pthread_mutex_unlock(&mutex);
    sorm_iterator_t *iterator;
    
    sorm_begin_read_transaction(_conn);
    device_t *device;
    ret = device_select_iterate_by_open(_conn, ALL_COLUMNS,
            NULL, &iterator);
    while (1) {
        device_select_iterate_by(iterator, &device);
        if (!device_select_iterate_more(iterator)) {
            break;
        }
        char string[1024];
        printf("select : %s\n", 
                device_to_string(device, string, 1024));
        device_free(device);
    }
    device_select_iterate_close(iterator);
    sorm_commit_transaction(_conn);
    
    
    //pthread_mutex_lock(&mutex);
    //condition_a = 1;
    //pthread_mutex_unlock(&mutex);
    //pthread_cond_signal(&cond_a);

    //printf("wait for input\n");
    //ret = getchar();
    //printf("continue running\n");
    
    //ret = device_select_by_uuid(_conn, ALL_COLUMNS, "uuid0",
    //        &device);
    //assert(ret == SORM_OK);
    //printf("get small num : %d\n", device->small_num);
    //device_free(device);
    //
    //sorm_commit_transaction(_conn);

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
    /* insert rows for select */
    int i;
    device_t *device;
    for(i = 0; i < 10; i ++)
    {
        if (i == 3) {
            continue;
        }
        device = device_new();

        device->id = i;
        device->id_stat = SORM_STAT_VALUED;
        sprintf(device->uuid,"uuid-%d", i);
        device->uuid_stat = SORM_STAT_VALUED;
        sprintf(device->name,"name-%d", i);
        device->name_stat = SORM_STAT_VALUED;
        sprintf(device->password, "passwd-%d", i);
        device->password_stat = SORM_STAT_VALUED;

        device_save(_conn, device);

        device_free(device);
    }
    
    ret = pthread_create(&p1, NULL, pthread_a_work, (void*)_conn);
    assert(ret == 0);
    ret = pthread_create(&p2, NULL, pthread_b_work, (void*)_conn);
    assert(ret == 0);
    
    pthread_join(p1, NULL);
    pthread_join(p2, NULL);
    
    sorm_iterator_t *iterator;
    
    ret = device_select_iterate_by_open(_conn, ALL_COLUMNS,
            NULL, &iterator);
    while (1) {
        device_select_iterate_by(iterator, &device);
        if (!device_select_iterate_more(iterator)) {
            break;
        }
        char string[1024];
        printf("select : %s\n", 
                device_to_string(device, string, 1024));
        device_free(device);
    }
    device_select_iterate_close(iterator);
    
    ret = device_delete_table(_conn);
    assert(ret == SORM_OK);
    sorm_close(_conn);
    
}
