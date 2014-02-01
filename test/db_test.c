/****************************************************************************
 *       Filename:  db_test.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/10/2013 02:38:44 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  
 ***************************************************************************/
#include <assert.h>
#include <pthread.h>

#include "sorm.h"
#include "device_sorm.h"
#include "volume_sorm.h"
#include "test_index_sorm.h"
#include "log.h"
#include "times.h"

#define DB_FILE "test.db"

pthread_cond_t cond_a = PTHREAD_COND_INITIALIZER;
int condition_a = 0, condition_b = 0;
pthread_cond_t cond_b = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_rwlock_t rwlock;

void* pthread_a_work(void* args)
{
    int ret, ret_val;
    sorm_connection_t *conn = (sorm_connection_t*)args;
    ret = sorm_open(DB_FILE, SORM_DB_SQLITE, 0, &rwlock, 
            SORM_ENABLE_RWLOCK, &conn);
    assert(ret == SORM_OK);
    
    sqlite3_stmt *stmt_handle = NULL;
    
    char *sql_stmt = "REPLACE INTO device ( id, uuid, name, password) VALUES ( 1, 'uuid-1', 'name-1', 'passwd')";
    
    ret = sqlite3_prepare(conn->sqlite3_handle, sql_stmt, SQL_STMT_MAX_LEN,
            &stmt_handle, NULL);

    if(ret != SQLITE_OK)
    {
        log_debug("sqlite3_prepare error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        return NULL;
    }
    log_debug("prepare stmt : %s", sql_stmt);
    
    //pthread_cond_signal(&cond_b);
    pthread_mutex_lock(&mutex);
    while (condition_a == 0) {
        pthread_cond_wait(&cond_a, &mutex); 
    }
    pthread_mutex_unlock(&mutex);
    
    ret = sqlite3_step(stmt_handle);

    if(ret != SQLITE_DONE)
    {
        log_debug("sqlite3_step error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        ret_val = SORM_DB_ERROR;
        goto DB_FINALIZE;
    }
    log_debug("step stmt : %s", sql_stmt);

    //ret = SQLITE_ROW;
    //while(ret == SQLITE_ROW)
    //{
    //    ret = sqlite3_step(stmt_handle);
    //    printf("%d : %d\n", ret, SQLITE_ROW);
    //    pthread_cond_signal(&cond_b);
    //    pthread_mutex_lock(&mutex);
    //    pthread_cond_wait(&cond_a, &mutex); 
    //    pthread_mutex_unlock(&mutex);

    //}

    if(ret != SQLITE_DONE)
    {
        log_debug("sqlite3_step error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        ret_val = SORM_DB_ERROR;
        goto DB_FINALIZE;
    }

    //log_debug("Success return");
    ret_val = SORM_OK;
    
    pthread_mutex_lock(&mutex);
    condition_b = 1;
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&cond_b);

DB_FINALIZE :
    ret = sqlite3_finalize(stmt_handle);
    if(ret != SQLITE_OK)
    {
        log_debug("sqlite3_finalize error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        return NULL;
    }
    
    log_debug("a finish");
    return NULL;
    
}

void* pthread_b_work(void* args)
{
    int ret, ret_val;
    sorm_connection_t *conn;
    ret = sorm_open(DB_FILE, SORM_DB_SQLITE, 0, NULL, 
            SORM_ENABLE_SEMAPHORE, &conn);
    assert(ret == SORM_OK);
    
    sqlite3_stmt *stmt_handle = NULL;
    char *sql_stmt = "REPLACE INTO device ( id, uuid, name, password) VALUES ( 2, 'uuid-2', 'name-2', 'passwd')";
    
    ret = sqlite3_prepare(conn->sqlite3_handle, sql_stmt, SQL_STMT_MAX_LEN,
            &stmt_handle, NULL);

    if(ret != SQLITE_OK)
    {
        log_debug("sqlite3_prepare error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        return NULL;
    }
    
    log_debug("prepare stmt : %s", sql_stmt);

    ret = sqlite3_step(stmt_handle);

    if(ret != SQLITE_DONE)
    {
        log_debug("sqlite3_step error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        ret_val = SORM_DB_ERROR;
        goto DB_FINALIZE;
    }
    
    log_debug("step stmt : %s", sql_stmt);
    
    pthread_mutex_lock(&mutex);
    condition_a = 1;
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&cond_a);
    pthread_mutex_lock(&mutex);
    while (condition_b == 0) {
        pthread_cond_wait(&cond_b, &mutex); 
    }
    pthread_mutex_unlock(&mutex);
    

    //log_debug("Success return");
    ret_val = SORM_OK;
    
DB_FINALIZE :
    ret = sqlite3_finalize(stmt_handle);
    if(ret != SQLITE_OK)
    {
        log_debug("sqlite3_finalize error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        return NULL;
    }

    log_debug("b finish");
    return NULL;

}

static void  _device_get_all(sorm_connection_t *conn) {
    sorm_iterator_t *iterator;
    device_t *device;
    int ret = device_select_iterate_by_open(conn, ALL_COLUMNS,
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

}

int main()
{
    sorm_connection_t *conn;
    pthread_t p1, p2;
    int ret;

    ret = sorm_init(0);
    pthread_rwlock_init(&rwlock, NULL);

    ret = sorm_open(DB_FILE, SORM_DB_SQLITE, 0, NULL, 
            0, &conn);
    assert(ret == SORM_OK);

    ret = test_index_create_table(conn);
    //ret = test_index_create_index(conn, "uuid");
    assert(ret == SORM_OK);
    //ret = test_index_create_index(conn, "name");
    //test_index_create_test_index(conn, COLUMN__DEVICE__SMALL_NUM);
    assert(ret == SORM_OK);

#define UPDATE_NUM 100000
    test_index_t *test_index = test_index_new();
    int i;
    sorm_begin_write_transaction(conn);
    for (i = 0; i < UPDATE_NUM; i ++) {
        test_index->id = i;
        test_index->id_stat = SORM_STAT_VALUED;
        sprintf(test_index->uuid,"uuid-%d", 1);
        test_index->uuid_stat = SORM_STAT_VALUED;
        sprintf(test_index->name,"name-%d", i);
        test_index->name_stat = SORM_STAT_VALUED;
        ret = test_index_insert(conn, test_index);
        assert(ret == SORM_OK);
    }
    sorm_commit_transaction(conn);

    times_init();

    //_test_index_get_all(conn);
    times_start();
    int num;
    for (i = 0; i < 1000; i ++) {
        ret = test_index_select_all_array_by(conn, ALL_COLUMNS, "name = 'name-1' AND uuid = 'uuid-1'", &num, &test_index);
        assert(ret == SORM_OK);
        assert(num == 1);
    }
    printf("timee : %f\n", times_end());
    
    //times_start();
    //for (i = 0; i < 1000; i ++) {
    //    ret = test_index_select_all_array_by(conn, ALL_COLUMNS, "name = 'name-1'", &num, &test_index);
    //    assert(ret == SORM_OK);
    //    assert(num == 1);
    //}
    //printf("timee : %f\n", times_end());
        //sqlite3_stmt *stmt_handle = NULL;
        //char sql_stmt[1024];
        //snprintf(sql_stmt, 1024, "UPDATE device SET small_num = 2 WHERE id = %d", i);
        //ret = sqlite3_prepare(conn->sqlite3_handle, sql_stmt, SQL_STMT_MAX_LEN,
        //        &stmt_handle, NULL);

        //if(ret != SQLITE_OK)
        //{
        //    log_debug("sqlite3_prepare error : %s", 
        //            sqlite3_errmsg(conn->sqlite3_handle));
        //    return 0;
        //}

        //ret = sqlite3_step(stmt_handle);
        //if(ret != SQLITE_DONE)
        //{
        //    log_debug("sqlite3_step error : %s", 
        //            sqlite3_errmsg(conn->sqlite3_handle));
        //    return SORM_DB_ERROR;
        //}

        //assert(sqlite3_changes(conn->sqlite3_handle) == 1);

        //ret = sqlite3_finalize(stmt_handle);
        //if(ret != SQLITE_OK)
        //{
        //    log_debug("sqlite3_finalize error : %s", 
        //            sqlite3_errmsg(conn->sqlite3_handle));
        //    return -1;
        //}

    //printf("use time : %f\n", times_end());

    //_device_get_all(conn);
    ret = test_index_delete_table(conn);
    assert(ret == SORM_OK);
    //ret = volume_delete_table(conn);
    //assert(ret == SORM_OK);
    
    sorm_close(conn);
    sorm_final();
}
