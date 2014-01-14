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
#include "log.h"

#define DB_FILE "test.db"
#define SEM_KEY 1025

pthread_cond_t cond_a = PTHREAD_COND_INITIALIZER;
int condition_a = 0, condition_b = 0;
pthread_cond_t cond_b = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* pthread_a_work(void* args)
{
    int ret, ret_val;
    sorm_connection_t *conn = (sorm_connection_t*)args;
    ret = sorm_open(DB_FILE, SORM_DB_SQLITE, SEM_KEY,
            SORM_ENABLE_SEMAPHORE, &conn);
    assert(ret == SORM_OK);
    
    sorm_run_stmt(conn, "BEGIN IMMEDIATE TRANSACTION");
    
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
    
    sorm_run_stmt(conn, "ROLLBACK TRANSACTION");
    
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
    ret = sorm_open(DB_FILE, SORM_DB_SQLITE, SEM_KEY,
            SORM_ENABLE_SEMAPHORE, &conn);
    assert(ret == SORM_OK);

    sorm_run_stmt(conn, "BEGIN IMMEDIATE TRANSACTION");
    
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
    
    sorm_run_stmt(conn, "COMMIT TRANSACTION");

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

int main()
{
    sorm_connection_t *conn;
    pthread_t p1, p2;
    int ret;

    ret = sorm_init(0);
    ret = sorm_open(DB_FILE, SORM_DB_SQLITE, SEM_KEY,
            SORM_ENABLE_SEMAPHORE, &conn);
    assert(ret == SORM_OK);

    ret = device_create_table(conn);
    assert(ret == SORM_OK);
    //ret = volume_create_table(conn);
    //assert(ret == SORM_OK);

    //volume = volume_new();
    //volume_set_id(volume, 1);
    //volume_set_uuid(volume, "uuid-1");
    //ret = volume_save(conn, volume);
    //assert(ret == SORM_OK);

    ret = pthread_create(&p1, NULL, pthread_a_work, (void*)conn);
    assert(ret == 0);
    ret = pthread_create(&p2, NULL, pthread_b_work, (void*)conn);
    assert(ret == 0);

    pthread_join(p1, NULL);
    pthread_join(p2, NULL);
    
#define DB_STRING_MAX_LEN 1024
    char string[DB_STRING_MAX_LEN];
    device_t *device;
    int num, i;
    ret = device_select_all_array_by(conn, ALL_COLUMNS, NULL, &num,  
            &device);
    for (i = 0; i < num; i ++) {
        log_debug("%s", device_to_string(&device[i], string, 
                    DB_STRING_MAX_LEN));
    }

    ret = device_delete_table(conn);
    assert(ret == SORM_OK);
    //ret = volume_delete_table(conn);
    //assert(ret == SORM_OK);
    
    sorm_close(conn);
    sorm_final();
}
