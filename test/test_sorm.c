/****************************************************************************
 *       Filename:  test_sorm.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/15/13 16:26:39
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *       Email:  never.wencan@gmail.com
 ***************************************************************************/
#include <CUnit/CUnit.h>

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>

#include "sorm.h"
#include "generate.h"
#include "volume_sorm.h"
#include "device_sorm.h"
#include "log.h"
#include "text_blob_sorm.h"

#define DB_FILE "sorm.db"
#define FILTER_MAX_LEN   127
#define sem_key 1024

typedef struct blob_s
{
    int i;
    double d;
}blob_t;

static sorm_connection_t *conn;
static device_t *device;

static int suite_sorm_init(void)
{
    int ret;

    if(sorm_init() != SORM_OK)
    {
        return -1;
    }

    ret = sorm_open(DB_FILE, SORM_DB_SQLITE, sem_key, NULL, 
            SORM_ENABLE_SEMAPHORE | SORM_ENABLE_FOREIGN_KEY, &conn);

    if(ret != SORM_OK)
    {
        return -1;
    }

    ret = device_create_table(conn);
    if(ret != SORM_OK)
    {
        return -1;
    }


    ret = volume_create_table(conn);
    if(ret != SORM_OK)
    {
        return -1;
    }

    ret = text_blob_create_table(conn);
    if(ret != SORM_OK)
    {
        return -1;
    }

    return 0;
}

static int _insert_device_range(int num)
{
    device_t *device;
    int i;

    for(i = 0; i < num; i ++)
    {
        device = device_new(NULL);

        device->id = i;
        device->id_stat = SORM_STAT_VALUED;
        sprintf(device->uuid,"uuid-%d", i);
        device->uuid_stat = SORM_STAT_VALUED;
        sprintf(device->name,"name-%d", i);
        device->name_stat = SORM_STAT_VALUED;
        sprintf(device->password, "passwd-%d", i);
        device->password_stat = SORM_STAT_VALUED;

        device_insert(conn, device);

        device_free(NULL, device);
    }
}

static int suite_sorm_final(void)
{
    int ret;
    int n;

    ret = volume_delete_table(conn);
    if(ret != SORM_OK)
    {
        return -1;
    }

    ret = device_delete_table(conn);
    if(ret != SORM_OK)
    {
        return -1;
    }

    ret = text_blob_delete_table(conn);
    if(ret != SORM_OK)
    {
        return -1;
    }

    ret = sorm_close(conn);

    if(ret != SORM_OK)
    {
        return -1;
    }
    sorm_final();

    return 0;
}

static void test_device_new(void)
{
    device = device_new(NULL);

    CU_ASSERT(device != NULL);

    device_free(NULL, device);
}

/* insert, select and delete */
static void test_device_ssd(void)
{
    int ret;
    device_t *get_device;
    device_t *to_del_device;
    int n;

    //insert a unset device
    device = device_new(NULL);
    ret = device_insert(conn, device);
    CU_ASSERT(ret == SORM_OK);

    device_set_id(device, 1);
    device_set_uuid(device, "123456");
    device_set_name(device, "test_device");
    device_set_password(device, "654321");

    ret = device_insert(conn, device);

    CU_ASSERT(ret == SORM_OK);

    ret = device_select_by_id(conn, NULL, ALL_COLUMNS, 1, &get_device);

    CU_ASSERT(ret == SORM_OK);

    CU_ASSERT(get_device != NULL);

    CU_ASSERT(device->id == get_device->id);
    CU_ASSERT(get_device->uuid != NULL);
    assert(get_device->uuid != NULL);
    CU_ASSERT(strcmp(device->uuid, get_device->uuid) == 0);
    CU_ASSERT(get_device->name != NULL);
    CU_ASSERT(strcmp(device->name, get_device->name) == 0);
    CU_ASSERT(get_device->password != NULL);
    CU_ASSERT(strcmp(device->password, get_device->password) == 0);
    device_free(NULL, get_device);

    to_del_device = device_new(NULL);
    device_set_id(to_del_device, 1);
    device_set_name(to_del_device, "test_devicefuck");
    device_set_password(to_del_device, "654321fuck");

    ret = device_delete(conn, to_del_device);
    CU_ASSERT(ret == SORM_OK);
    ret = device_select_by_id(conn, NULL, ALL_COLUMNS, 1, &get_device);
    assert(ret == SORM_OK);
    device_free(NULL, get_device);

    device_set_uuid(to_del_device, "123456");
    device_set_name(to_del_device, "test_device");
    device_set_password(to_del_device, "654321");
    ret = device_delete(conn, to_del_device);
    CU_ASSERT(ret == SORM_OK);

    device_free(NULL, to_del_device);

    ret = device_select_by_id(conn, NULL, ALL_COLUMNS, 1, &get_device);
    CU_ASSERT(ret == SORM_NOEXIST);
    CU_ASSERT(get_device == NULL);

    //delete a unset device
    to_del_device = device_new(NULL);
    ret = device_delete(conn, to_del_device);
    CU_ASSERT(ret == SORM_OK);
    device_free(NULL, to_del_device);
    
    ret = device_select_by_id(conn, NULL, "", 1, &get_device);
    CU_ASSERT(ret == SORM_COLUMNS_NAME_EMPTY);
    CU_ASSERT(get_device == NULL);
    
    ret = device_select_all_array_by(conn, NULL, "*", "", &n, &get_device);
    CU_ASSERT(ret == SORM_FILTER_EMPTY);
    CU_ASSERT(get_device == NULL);
    
    ret = device_delete_by(conn, NULL);
    CU_ASSERT(ret == SORM_OK);
}

static void test_device_update(void)
{
    int ret;
    device_t *update_device, *get_device;

    device_set_id(device, 1);
    ret = device_insert(conn, device);
    CU_ASSERT(ret == SORM_OK);

    ret = device_select_by_id(conn, NULL, ALL_COLUMNS, 1, &update_device);
    CU_ASSERT(ret == SORM_OK);

    device_set_uuid(update_device, "456i789");
    device_set_password(update_device, "789123");

    ret = device_replace(conn, update_device);
    CU_ASSERT(ret == SORM_OK);

    ret = device_select_by_id(conn, NULL, ALL_COLUMNS, 1, &get_device);
    CU_ASSERT(ret == SORM_OK);

    CU_ASSERT(update_device->id == get_device->id);
    CU_ASSERT(get_device->uuid != NULL);
    assert(get_device->uuid != NULL);
    CU_ASSERT(strcmp(update_device->uuid, get_device->uuid) == 0);
    CU_ASSERT(get_device->name != NULL);
    CU_ASSERT(strcmp(update_device->name, get_device->name) == 0);
    CU_ASSERT(get_device->password != NULL);
    CU_ASSERT(strcmp(update_device->password, get_device->password) == 0);

    device_free(NULL, update_device);
    device_free(NULL, get_device);

    ret = device_delete_by_id(conn, 1);
}

static void test_device_free(void)
{
    device_free(NULL, device);
    device = NULL;

    CU_ASSERT(device == NULL);
}

static int _insert_long_row_into_device(int id, char *name)
{
    char sql_stmt[SQL_STMT_MAX_LEN + 1] = "";
    sqlite3_stmt *stmt_handle;
    int ret, ret_val;

    /* sql statment : "UPDATE table_name "*/
    snprintf(sql_stmt, SQL_STMT_MAX_LEN + 1, 
            "REPLACE INTO device VALUES (%d, '%s', 'too_long', '123', "
            "123, 123)", id, name);

    ret = sqlite3_prepare(conn->sqlite3_handle, sql_stmt, 
            SQL_STMT_MAX_LEN, &stmt_handle, NULL);

    if(ret != SQLITE_OK)
    {
        log_debug("sqlite3_prepare error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        return -1;
    }

    ret = SQLITE_BUSY;
    while(ret == SQLITE_BUSY)
    {
        ret = sqlite3_step(stmt_handle);
    }

    if(ret != SQLITE_DONE)
    {
        log_debug("sqlite3_step error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        ret_val = -1;
        goto DB_FINALIZE;
    }

    ret_val = 0;

DB_FINALIZE :
    ret = sqlite3_finalize(stmt_handle);
    if(ret != SQLITE_OK)
    {
        log_debug("sqlite3_finalize error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        return -1;

    }
    return ret_val;

}

static void test_device_select_too_long(void)
{
    /* select a too long value string in db, need 
     * insert rows : PK = 200, name_len = 31;
     *           PK = 201, name_len = 32;
     *           PK = 202, name_len = 33;
     * in db by using cmd*/
    device_t *get_device;
    int ret;

    _insert_long_row_into_device(1, "1234567890123456789012345678901");
    _insert_long_row_into_device(2, "12345678901234567890123456789012");
    _insert_long_row_into_device(3, "123456789012345678901234567890123");

    ret = device_select_by_id(conn, NULL, ALL_COLUMNS, 1, &get_device);
    device_free(NULL, get_device);
    CU_ASSERT(ret == SORM_OK);
    ret = device_select_by_id(conn, NULL, ALL_COLUMNS, 2, &get_device);
    CU_ASSERT(ret == SORM_TOO_LONG);
    ret = device_select_by_id(conn, NULL, ALL_COLUMNS, 3, &get_device);
    CU_ASSERT(ret == SORM_TOO_LONG);

    ret = device_delete_by_id(conn, 1);
    CU_ASSERT(ret == SORM_OK);
    ret = device_delete_by_id(conn, 2);
    CU_ASSERT(ret == SORM_OK);
    ret = device_delete_by_id(conn, 3);
    CU_ASSERT(ret == SORM_OK);
}
static void test_device_select(void)
{
    device_t *device;
    int i;
    int amount = 10;
    int n, ret;
    device_t *select_device;
    char filter[FILTER_MAX_LEN + 1];

    /*insert some value for select*/
    device = device_new(NULL);
    assert(device != NULL);
    for(i = 0; i < amount; i ++)
    {
        device_set_id(device, i);
        sprintf(device->uuid,"uuid-%d", i);
        device->uuid_stat = SORM_STAT_VALUED;
        sprintf(device->name,"name-%d", i);
        device->name_stat = SORM_STAT_VALUED;
        sprintf(device->password, "passwd-%d", i);
        device->password_stat = SORM_STAT_VALUED;
        device_insert(conn, device);
    }
    /*****************************/
    /* test select_some_array_by */
    /*****************************/
    //printf("Start select half by select_some_array_by without filter\n");
    n = amount / 2;
    ret = device_select_some_array_by(conn, NULL, ALL_COLUMNS, NULL, &n, &select_device);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == (amount / 2));
    for(i = 0; i < n; i ++)
    {
        CU_ASSERT(select_device[i].id == i);
        sprintf(device->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(device->uuid, select_device[i].uuid) == 0);
        sprintf(device->name,"name-%d", i);
        CU_ASSERT(strcmp(device->name, select_device[i].name) == 0);
        sprintf(device->password, "passwd-%d", i);
        CU_ASSERT(strcmp(device->password, select_device[i].password) == 0);
    }
    device_free(NULL, select_device);

    //printf("Start select 0 by select_some_array_by wihout filter\n");
    n = 0;
    /*avoid to get the row for test_device_select_too_long */
    ret = device_select_some_array_by(conn, NULL,  ALL_COLUMNS, NULL, &n, &select_device);
    CU_ASSERT(ret == SORM_NOEXIST);
    CU_ASSERT(n == 0);
    CU_ASSERT(select_device == NULL);

    //printf("Start select half by select_some_array_by with filter\n");
    n = amount * 2;
    sprintf(filter, COLUMN__DEVICE__ID" < %d", 5);
    ret = device_select_some_array_by(conn, NULL,  ALL_COLUMNS, filter, &n, &select_device);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == 5);
    for(i = 0; i < n; i ++)
    {
        CU_ASSERT(select_device[i].id == i);
        sprintf(device->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(device->uuid, select_device[i].uuid) == 0);
        sprintf(device->name,"name-%d", i);
        CU_ASSERT(strcmp(device->name, select_device[i].name) == 0);
        sprintf(device->password, "passwd-%d", i);
        CU_ASSERT(strcmp(device->password, select_device[i].password) == 0);

    }
    device_free(NULL, select_device);

    //printf("Start select all by select_some_array_by without filter\n");
    n = amount * 2;
    ret = device_select_some_array_by(conn, NULL,  ALL_COLUMNS, NULL, &n, &select_device);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == amount);
    for(i = 0; i < n; i ++)
    {
        CU_ASSERT(select_device[i].id == i);
        sprintf(device->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(device->uuid, select_device[i].uuid) == 0);
        sprintf(device->name,"name-%d", i);
        CU_ASSERT(strcmp(device->name, select_device[i].name) == 0);
        sprintf(device->password, "passwd-%d", i);
        CU_ASSERT(strcmp(device->password, select_device[i].password) == 0);

    }
    device_free(NULL, select_device);

    /*****************************/
    /* test select_some_list_by */
    /*****************************/
    sorm_list_t *select_device_list, *list_head;

    //printf("Start select half by select_some_list_by without filter\n");
    n = amount / 2;
    ret = device_select_some_list_by(conn, NULL,  ALL_COLUMNS, NULL, &n, &select_device_list);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == (amount / 2));
    i = 0;
    device_list_for_each(select_device, list_head, select_device_list)
    {
        CU_ASSERT(select_device->id == i);
        sprintf(device->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(device->uuid, select_device->uuid) == 0);
        sprintf(device->name,"name-%d", i);
        CU_ASSERT(strcmp(device->name, select_device->name) == 0);
        sprintf(device->password, "passwd-%d", i);
        CU_ASSERT(strcmp(device->password, select_device->password) == 0);
        i ++;
    }
    sorm_list_free(NULL, select_device_list);

    //printf("Start select 0 by select_some_list_by wihout filter\n");
    n = 0;
    /*avoid to get the row for test_device_select_too_long */
    ret = device_select_some_list_by(conn, NULL,  ALL_COLUMNS, NULL, &n, &select_device_list);
    CU_ASSERT(ret == SORM_NOEXIST);
    CU_ASSERT(n == 0);

    //printf("Start select half by select_some_list_by with filter\n");
    n = amount * 2;
    sprintf(filter, COLUMN__DEVICE__ID" < %d", 5);
    ret = device_select_some_list_by(conn, NULL,  ALL_COLUMNS, filter, &n, &select_device_list);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == 5);
    i = 0;
    device_list_for_each(select_device, list_head, select_device_list)
    {
        CU_ASSERT(select_device->id == i);
        sprintf(device->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(device->uuid, select_device->uuid) == 0);
        sprintf(device->name,"name-%d", i);
        CU_ASSERT(strcmp(device->name, select_device->name) == 0);
        sprintf(device->password, "passwd-%d", i);
        CU_ASSERT(strcmp(device->password, select_device->password) == 0);
        i ++;
    }
    sorm_list_free(NULL, select_device_list);

    //printf("Start select all by select_some_list_by without filter\n");
    n = amount * 2;
    ret = device_select_some_list_by(conn, NULL,  ALL_COLUMNS, NULL, &n, &select_device_list);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == amount);
    i = 0;
    device_list_for_each(select_device, list_head, select_device_list)
    {
        CU_ASSERT(select_device->id == i);
        sprintf(device->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(device->uuid, select_device->uuid) == 0);
        sprintf(device->name,"name-%d", i);
        CU_ASSERT(strcmp(device->name, select_device->name) == 0);
        sprintf(device->password, "passwd-%d", i);
        CU_ASSERT(strcmp(device->password, select_device->password) == 0);
        i ++;
    }
    sorm_list_free(NULL, select_device_list);

    /*****************************/
    /* test select_all_array_by */
    /*****************************/
    //printf("Start select half by select_all_array_by with filter\n");
    n = 0;
    sprintf(filter, COLUMN__DEVICE__ID" < %d", 5);
    ret = device_select_all_array_by(conn, NULL,  ALL_COLUMNS, filter, &n, &select_device);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == 5);
    for(i = 0; i < n; i ++)
    {
        CU_ASSERT(select_device[i].id == i);
        sprintf(device->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(device->uuid, select_device[i].uuid) == 0);
        sprintf(device->name,"name-%d", i);
        CU_ASSERT(strcmp(device->name, select_device[i].name) == 0);
        sprintf(device->password, "passwd-%d", i);
        CU_ASSERT(strcmp(device->password, select_device[i].password) == 0);
    }
    assert(device != NULL);
    device_free(NULL, select_device);

    //printf("Start select all by select_all_array_by without filter\n");
    n = 0;
    ret = device_select_all_array_by(conn, NULL,  ALL_COLUMNS, NULL, &n, &select_device);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == amount);
    for(i = 0; i < n; i ++)
    {
        CU_ASSERT(select_device[i].id == i);
        sprintf(device->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(device->uuid, select_device[i].uuid) == 0);
        sprintf(device->name,"name-%d", i);
        CU_ASSERT(strcmp(device->name, select_device[i].name) == 0);
        sprintf(device->password, "passwd-%d", i);
        CU_ASSERT(strcmp(device->password, select_device[i].password) == 0);
    }
    assert(device != NULL);
    device_free(NULL, select_device);

    /*****************************/
    /* test select_all_list_by */
    /*****************************/
    //printf("Start select half by select_all_list_by with filter\n");
    n = 0;
    sprintf(filter, COLUMN__DEVICE__ID" < %d", 5);
    ret = device_select_all_list_by(conn, NULL,  ALL_COLUMNS, filter, &n, &select_device_list);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == 5);
    i = 0;
    device_list_for_each(select_device, list_head, select_device_list)
    {
        select_device = (device_t*)list_head->data;
        CU_ASSERT(select_device->id == i);
        sprintf(device->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(device->uuid, select_device->uuid) == 0);
        sprintf(device->name,"name-%d", i);
        CU_ASSERT(strcmp(device->name, select_device->name) == 0);
        sprintf(device->password, "passwd-%d", i);
        CU_ASSERT(strcmp(device->password, select_device->password) == 0);
        i++;
    }
    assert(device != NULL);
    sorm_list_free(NULL, select_device_list);

    //printf("Start select all by select_all_list_by without filter\n");
    n = 0;
    ret = device_select_all_list_by(conn, NULL,  ALL_COLUMNS, NULL, &n, &select_device_list);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == amount);
    i = 0;
    device_list_for_each(select_device, list_head, select_device_list)
    {
        select_device = (device_t*)list_head->data;
        CU_ASSERT(select_device->id == i);
        sprintf(device->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(device->uuid, select_device->uuid) == 0);
        sprintf(device->name,"name-%d", i);
        CU_ASSERT(strcmp(device->name, select_device->name) == 0);
        sprintf(device->password, "passwd-%d", i);
        CU_ASSERT(strcmp(device->password, select_device->password) == 0);
        i++;
    }
    assert(device != NULL);
    sorm_list_free(NULL, select_device_list);

    /* delete insert values */
    for(i = 0; i < amount; i ++)
    {
        device_delete_by_id(conn, i);
    }

    /*****************************/
    /* select form empty */
    /*****************************/
    //printf("select by select_some_array_by from empty table\n");
    n = amount / 2;
    ret = device_select_some_array_by(conn, NULL,  ALL_COLUMNS, NULL, &n, NULL);
    CU_ASSERT(ret == SORM_NOEXIST);
    CU_ASSERT(n == 0);

    //printf("select by select_some_list_by from empty table\n");
    n = amount / 2;
    ret = device_select_some_list_by(conn, NULL,  ALL_COLUMNS, NULL, &n, NULL);
    CU_ASSERT(ret == SORM_NOEXIST);
    CU_ASSERT(n == 0);

    //printf("select by select_all_array_by from empty table\n");
    ret = device_select_all_array_by(conn, NULL,  ALL_COLUMNS, NULL, &n, NULL);
    CU_ASSERT(ret == SORM_NOEXIST);
    CU_ASSERT(n == 0);

    //printf("select by select_all_list_by from empty table\n");
    ret = device_select_all_list_by(conn, NULL,  ALL_COLUMNS, NULL, &n, NULL);
    CU_ASSERT(ret == SORM_NOEXIST);
    CU_ASSERT(n == 0);

    device_free(NULL, device);
}

static void test_device_sql_too_long(void)
{
    /* construct a too long sql when select*/
    device_t *get_device;
    int ret;
    char *filter;
    int n = 1;
    device_t *select_device;

    filter = (char *)malloc(1024);
    filter[1022] = 0;

    ret = device_select_some_array_by(conn, NULL,  ALL_COLUMNS, filter, &n, &select_device);
    CU_ASSERT(ret == SORM_TOO_LONG);
}

static void test_device_noexist(void)
{
    /* select an nonexit PK */
    device_t *get_device;
    int ret;

    ret = device_select_by_id(conn, NULL, ALL_COLUMNS, 137, &get_device);
    CU_ASSERT(get_device == NULL);
    CU_ASSERT(ret == SORM_NOEXIST);
}

static void test_device_auto_pk(void)
{
    device_t *device;
    int i;
    int amount = 10;
    int n, ret;

    device = device_new(NULL);
    assert(device != NULL);
    for(i = 0; i < amount; i ++)
    {
        sprintf(device->uuid,"uuid-%d", i);
        device->uuid_stat = SORM_STAT_VALUED;
        sprintf(device->name,"name-%d", i);
        device->name_stat = SORM_STAT_VALUED;
        sprintf(device->password, "passwd-%d", i);
        device->password_stat = SORM_STAT_VALUED;
        device_insert(conn, device);
    }

    for(i = 0; i < amount; i ++)
    {
        ret = device_select_by_id(conn, NULL, ALL_COLUMNS, i, NULL);
        assert(ret == SORM_OK);
    }

    for(i = 0; i < amount; i ++)
    {
        device_delete_by_id(conn, i);
    }

    device_free(NULL, device);
}

static void test_transaction(void)
{
    device_t *device;
    int i;
    int amount = 10;
    int n, ret;
    device_t *select_device, *get_device;
    char filter[FILTER_MAX_LEN + 1];


    char buf[123];
    ret = sorm_begin_write_transaction(conn);
    for(i = 0; i < 10; i ++)
    {
        device = device_new(NULL);
        sprintf(buf, "uuid-%d", i);
        device_set_uuid(device, buf);
        sprintf(buf, "name-%d", i);
        device_set_name(device,buf);
        ret = device_insert(conn, device);
        CU_ASSERT(ret == SORM_OK);
        device_free(NULL, device);
    }

    ret = sorm_commit_transaction(conn);

    ret = device_select_all_array_by(conn, NULL,  "*", NULL, &n, &select_device);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == 10);
    for(i = 0; i < 10; i ++)
    {
        sprintf(buf, "uuid-%d", i);
        CU_ASSERT(strcmp(select_device[i].uuid, buf) == 0);
        sprintf(buf, "name-%d", i);
        CU_ASSERT(strcmp(select_device[i].name, buf) == 0);
    }
    device_free(NULL, select_device);

    device = device_new(NULL);

    //printf("commit test\n");
    ret = sorm_begin_write_transaction(conn);
    CU_ASSERT(ret == SORM_OK);
    device->id = 1;
    device->id_stat = SORM_STAT_VALUED;
    sprintf(device->uuid,"uuid");
    device->uuid_stat = SORM_STAT_VALUED;
    sprintf(device->name,"name");
    device->name_stat = SORM_STAT_VALUED;
    sprintf(device->password, "passwd");
    device->password_stat = SORM_STAT_VALUED;
    ret = device_replace(conn, device);
    CU_ASSERT(ret == SORM_OK);

    ret = sorm_commit_transaction(conn);

    ret = device_select_by_id(conn, NULL, ALL_COLUMNS, 1, &get_device);
    CU_ASSERT(ret == SORM_OK);
    ret = device_delete(conn, get_device);
    CU_ASSERT(ret == SORM_OK);
    device_free(NULL, get_device);

    //printf("rollback test\n");
    ret = sorm_begin_write_transaction(conn);
    CU_ASSERT(ret == SORM_OK);
    device->id = 1;
    device->id_stat = SORM_STAT_VALUED;
    sprintf(device->uuid,"uuid");
    device->uuid_stat = SORM_STAT_VALUED;
    sprintf(device->name,"name");
    device->name_stat = SORM_STAT_VALUED;
    sprintf(device->password, "passwd");
    device->password_stat = SORM_STAT_VALUED;
    ret = device_insert(conn, device);
    CU_ASSERT(ret == SORM_OK);

    ret = sorm_rollback_transaction(conn);

    ret = device_select_by_id(conn, NULL, ALL_COLUMNS, 1, &get_device);
    CU_ASSERT(ret == SORM_NOEXIST);
    CU_ASSERT(get_device == NULL);

    //printf("nest test 1\n");
    ret = sorm_begin_write_transaction(conn);
    CU_ASSERT(ret == SORM_OK);
    device->id = 1;
    device->id_stat = SORM_STAT_VALUED;
    sprintf(device->uuid,"uuid");
    device->uuid_stat = SORM_STAT_VALUED;
    sprintf(device->name,"name");
    device->name_stat = SORM_STAT_VALUED;
    sprintf(device->password, "passwd");
    device->password_stat = SORM_STAT_VALUED;
    ret = device_insert(conn, device);
    CU_ASSERT(ret == SORM_OK);

    ret = sorm_begin_write_transaction(conn);
    CU_ASSERT(ret == SORM_OK);
    device->id = 2;
    device->id_stat = SORM_STAT_VALUED;
    sprintf(device->uuid,"uuid2");
    device->uuid_stat = SORM_STAT_VALUED;
    sprintf(device->name,"name2");
    device->name_stat = SORM_STAT_VALUED;
    sprintf(device->password, "passwd2");
    device->password_stat = SORM_STAT_VALUED;
    ret = device_replace(conn, device);
    CU_ASSERT(ret == SORM_OK);

    ret = sorm_rollback_transaction(conn);

    ret = sorm_commit_transaction(conn);

    ret = device_select_by_id(conn, NULL, ALL_COLUMNS, 1, &get_device);
    CU_ASSERT(ret == SORM_OK);
    ret = device_delete(conn, get_device);
    CU_ASSERT(ret == SORM_OK);
    device_free(NULL, get_device);
    ret = device_select_by_id(conn, NULL, ALL_COLUMNS, 2, &get_device);
    CU_ASSERT(ret == SORM_OK);
    ret = device_delete(conn, get_device);
    CU_ASSERT(ret == SORM_OK);
    device_free(NULL, get_device);

    //printf("nest test 2\n");
    ret = sorm_begin_write_transaction(conn);
    CU_ASSERT(ret == SORM_OK);
    device->id = 1;
    device->id_stat = SORM_STAT_VALUED;
    sprintf(device->uuid,"uuid");
    device->uuid_stat = SORM_STAT_VALUED;
    sprintf(device->name,"name");
    device->name_stat = SORM_STAT_VALUED;
    sprintf(device->password, "passwd");
    device->password_stat = SORM_STAT_VALUED;
    ret = device_insert(conn, device);
    CU_ASSERT(ret == SORM_OK);

    ret = sorm_begin_write_transaction(conn);
    CU_ASSERT(ret == SORM_OK);
    device->id = 2;
    device->id_stat = SORM_STAT_VALUED;
    sprintf(device->uuid,"uuid2");
    device->uuid_stat = SORM_STAT_VALUED;
    sprintf(device->name,"name2");
    device->name_stat = SORM_STAT_VALUED;
    sprintf(device->password, "passwd2");
    device->password_stat = SORM_STAT_VALUED;
    ret = device_insert(conn, device);
    CU_ASSERT(ret == SORM_OK);

    ret = sorm_commit_transaction(conn);

    ret = sorm_rollback_transaction(conn);

    ret = device_select_by_id(conn, NULL, ALL_COLUMNS, 1, &get_device);
    CU_ASSERT(ret == SORM_NOEXIST);
    CU_ASSERT(get_device == NULL);
    ret = device_select_by_id(conn, NULL, ALL_COLUMNS, 2, &get_device);
    CU_ASSERT(ret == SORM_NOEXIST);
    CU_ASSERT(get_device == NULL);
    
    ret = device_delete_by(conn, COLUMN__DEVICE__ID" <= 10");

    device_free(NULL, device);
}

static void test_select_columns()
{
    device_t *device;
    int i;
    int amount = 10;
    int n, ret;
    device_t *select_device;
    sorm_list_t *device_list;
    char filter[FILTER_MAX_LEN + 1];

    /*insert some value for select*/
    device = device_new(NULL);
    assert(device != NULL);
    for(i = 0; i < amount; i ++)
    {
        device->id = i;
        device->id_stat = SORM_STAT_VALUED;
        sprintf(device->uuid,"uuid-%d", i);
        device->uuid_stat = SORM_STAT_VALUED;
        sprintf(device->name,"name-%d", i);
        device->name_stat = SORM_STAT_VALUED;
        sprintf(device->password, "passwd-%d", i);
        device->password_stat = SORM_STAT_VALUED;
        device_insert(conn, device);
    }

    //printf("select uuid\n");
    ret = device_select_all_array_by(conn, NULL,  COLUMN__DEVICE__UUID, NULL, &n, &select_device);
    CU_ASSERT(n == amount);
    for(i = 0; i < n; i ++)
    {
        sprintf(device->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(device->uuid, select_device[i].uuid) == 0);
    }
    device_free(NULL, select_device);

    //printf("select device.uuid\n");
    ret = device_select_all_array_by(conn, NULL,  COLUMN__DEVICE__UUID, NULL, &n, &select_device);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == amount);
    for(i = 0; i < n; i ++)
    {
        sprintf(device->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(device->uuid, select_device[i].uuid) == 0);
    }
    device_free(NULL, select_device);

    //printf("select device.uuid and id \n");
    ret = device_select_all_array_by(conn, NULL,  COLUMN__DEVICE__ID", "COLUMN__DEVICE__UUID, NULL, &n, &select_device);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == amount);
    for(i = 0; i < n; i ++)
    {
        CU_ASSERT(select_device[i].id == i);
        sprintf(device->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(device->uuid, select_device[i].uuid) == 0);
    }
    device_free(NULL, select_device);

    //printf("select device.uuid and id \n");
    ret = device_select_all_array_by(conn, NULL,  COLUMN__DEVICE__ID", "COLUMN__DEVICE__UUID, NULL, &n, &select_device);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == amount);
    for(i = 0; i < n; i ++)
    {
        CU_ASSERT(select_device[i].id == i);
        sprintf(device->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(device->uuid, select_device[i].uuid) == 0);
    }
    device_free(NULL, select_device);

    //printf("select * and id \n");
    ret = device_select_all_array_by(conn, NULL,  ALL_COLUMNS", "COLUMN__DEVICE__ID, NULL, &n, &select_device);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == amount);
    for(i = 0; i < n; i ++)
    {
        CU_ASSERT(select_device[i].id == i);
        sprintf(device->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(device->uuid, select_device[i].uuid) == 0);
        sprintf(device->name,"name-%d", i);
        CU_ASSERT(strcmp(device->name, select_device[i].name) == 0);
        sprintf(device->password, "passwd-%d", i);
        CU_ASSERT(strcmp(device->password, select_device[i].password) == 0);
    }
    device_free(NULL, select_device);


    //printf("select invalid column \n");
    /* TODO select not ok */
    ret = device_select_some_array_by(conn, NULL,  "invalid", NULL, &n, &select_device);
    CU_ASSERT(ret == SORM_DB_ERROR);
    CU_ASSERT(select_device == NULL);
    ret = device_select_all_array_by(conn, NULL,  "invalid", NULL, &n, &select_device);
    CU_ASSERT(ret == SORM_DB_ERROR);
    CU_ASSERT(select_device == NULL);
    ret = device_select_some_list_by(conn, NULL,  "invalid", NULL, &n, &device_list);
    CU_ASSERT(ret == SORM_DB_ERROR);
    CU_ASSERT(select_device == NULL);
    ret = device_select_all_list_by(conn, NULL,  "invalid", NULL, &n, &device_list);
    CU_ASSERT(ret == SORM_DB_ERROR);
    CU_ASSERT(select_device == NULL);

    for(i = 0; i < amount; i ++)
    {
        device_delete_by_id(conn, i);
    }
    device_free(NULL, device);
    device = NULL;
}

static void test_delete_by_id(void)
{
    device_t *device;
    int ret;
    device_t *select_device;

    /*insert some value for select*/
    device = device_new(NULL);

    device->id = 1;
    device->id_stat = SORM_STAT_VALUED;
    sprintf(device->uuid,"uuid");
    device->uuid_stat = SORM_STAT_VALUED;
    sprintf(device->name,"name");
    device->name_stat = SORM_STAT_VALUED;
    sprintf(device->password, "passwd");
    device->password_stat = SORM_STAT_VALUED;
    device_insert(conn, device);
    device_free(NULL, device);

    ret = device_delete_by_id(conn, 1);
    CU_ASSERT(ret == SORM_OK);

    ret = device_select_by_id(conn, NULL, ALL_COLUMNS, 1, &select_device);
    CU_ASSERT(ret == SORM_NOEXIST);
    CU_ASSERT(select_device == NULL);

    device_free(NULL, select_device);
}

static void test_delete_by(void)
{
    device_t *device;
    int ret, n;

    /*insert some value for select*/
    _insert_device_range(10);
    ret = device_delete_by(conn, NULL);
    CU_ASSERT(ret == SORM_OK);
    ret = device_select_all_array_by(conn, NULL,  DEVICE__ALL_COLUMNS, NULL, &n, NULL);
    CU_ASSERT(ret == SORM_NOEXIST);
    CU_ASSERT(n == 0);
    
    _insert_device_range(10);
    ret = device_select_all_array_by(conn, NULL,  DEVICE__ALL_COLUMNS, NULL, &n, NULL);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == 10);

    ret = device_delete_by(conn, COLUMN__DEVICE__ID " <= 4");
    CU_ASSERT(ret == SORM_OK);

    ret = device_select_all_array_by(conn, NULL,  DEVICE__ALL_COLUMNS, NULL, &n, NULL);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == 5);

    ret = device_delete_by(conn, COLUMN__DEVICE__ID " <= 9");
    CU_ASSERT(ret == SORM_OK);
    ret = device_select_all_array_by(conn, NULL,  DEVICE__ALL_COLUMNS, NULL, &n, NULL);
    CU_ASSERT(ret == SORM_NOEXIST);
    CU_ASSERT(n == 0);
}

static void test_select_by_column(void)
{
    device_t *device;
    int ret;
    device_t *select_device;

    /*insert some value for select*/
    device = device_new(NULL);

    device->id = 1;
    device->id_stat = SORM_STAT_VALUED;
    sprintf(device->uuid,"uuid");
    device->uuid_stat = SORM_STAT_VALUED;
    sprintf(device->name,"name");
    device->name_stat = SORM_STAT_VALUED;
    sprintf(device->password, "passwd");
    device->password_stat = SORM_STAT_VALUED;
    device_insert(conn, device);

    ret = device_select_by_uuid(conn, NULL, ALL_COLUMNS, "uuid", &select_device);
    CU_ASSERT(ret == SORM_OK);

    CU_ASSERT(device->id == select_device->id);
    CU_ASSERT(strcmp(device->uuid, select_device->uuid) == 0);
    CU_ASSERT(strcmp(device->name, select_device->name) == 0);
    CU_ASSERT(strcmp(device->password, select_device->password) == 0);

    device_free(NULL, device);
    device_free(NULL, select_device);

    ret = device_delete_by_id(conn, 1);
    CU_ASSERT(ret == SORM_OK);
}

static void test_select_null(void)
{
    device_t *device;
    int i;
    int amount = 10;
    int n, ret;
    device_t *select_device;
    char filter[FILTER_MAX_LEN + 1];

    /*insert some value for select*/
    device = device_new(NULL);
    assert(device != NULL);
    for(i = 0; i < amount; i ++)
    {
        device->id = i;
        device->id_stat = SORM_STAT_VALUED;
        sprintf(device->uuid,"uuid-%d", i);
        device->uuid_stat = SORM_STAT_VALUED;
        sprintf(device->name,"name-%d", i);
        device->name_stat = SORM_STAT_VALUED;
        sprintf(device->password, "passwd-%d", i);
        device->password_stat = SORM_STAT_VALUED;
        device_insert(conn, device);
    }

    n = amount / 2;
    ret = device_select_some_array_by(conn, NULL,  ALL_COLUMNS, NULL, &n, NULL);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == (amount / 2));

    n = amount / 2;
    ret = device_select_some_list_by(conn, NULL,  ALL_COLUMNS, NULL, &n, NULL);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == (amount / 2));

    ret = device_select_all_array_by(conn, NULL,  ALL_COLUMNS, NULL, &n, NULL);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == amount);

    ret = device_select_all_list_by(conn, NULL,  ALL_COLUMNS, NULL, &n, NULL);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == amount);

    for(i = 0; i < amount; i ++)
    {
        device_delete_by_id(conn, i);
    }

    device_free(NULL, device);
}

static void test_sorm_select_by_join()
{
    device_t *device;
    volume_t *volume;
    int i, n, ret;
    device_t *select_device;
    volume_t *select_volume;
    int device_index[6]={0, 0, 1, 1, 2, 3};
    int volume_index[6]={0, 3, 1, 4, 2, 0};

    /* insert rows for select */
    for(i = 0; i < 4; i ++)
    {
        device = device_new(NULL);

        device->id = i;
        device->id_stat = SORM_STAT_VALUED;
        sprintf(device->uuid,"uuid-%d", i);
        device->uuid_stat = SORM_STAT_VALUED;
        sprintf(device->name,"name-%d", i);
        device->name_stat = SORM_STAT_VALUED;
        sprintf(device->password, "passwd-%d", i);
        device->password_stat = SORM_STAT_VALUED;

        device_insert(conn, device);

        device_free(NULL, device);
    }

    for(i = 0; i < 5; i ++)
    {
        volume = volume_new(NULL);

        volume->id = i;
        volume->id_stat = SORM_STAT_VALUED;
        volume->device_id = i % 3;
        volume->device_id_stat = SORM_STAT_VALUED;
        sprintf(volume->uuid, "uuid-%d", i);
        volume->uuid_stat = SORM_STAT_VALUED;
        sprintf(volume->drive, "drive-%d", i);
        volume->drive_stat = SORM_STAT_VALUED;
        sprintf(volume->label, "label-%d", i);
        volume->label_stat = SORM_STAT_VALUED;

        volume_insert(conn, volume);
        volume_free(NULL, volume);
    }
    volume = volume_new(NULL);
    i = 6;
    volume->id = i;
    volume->id_stat = SORM_STAT_VALUED;
    volume->device_id = 100;
    volume->device_id_stat = SORM_STAT_VALUED;
    sprintf(volume->uuid, "uuid-%d", i);
    volume->uuid_stat = SORM_STAT_VALUED;
    sprintf(volume->drive, "drive-%d", i);
    volume->uuid_stat = SORM_STAT_VALUED;
    sprintf(volume->label, "label-%d", i);
    volume->label_stat = SORM_STAT_VALUED;
    volume_insert(conn, volume);
    volume_free(NULL, volume);

    device = device_new(NULL);
    volume = volume_new(NULL);
    //printf("test inner join\n");
    ret = sorm_select_all_array_by_join(conn, NULL,  ALL_COLUMNS, DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, NULL, &n,
            (sorm_table_descriptor_t**)&select_device, (sorm_table_descriptor_t**)&select_volume);
    
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == 5);
    for(i = 0; i < n; i ++)
    {
        CU_ASSERT(select_device[i].id == i % 3);
        sprintf(device->uuid,"uuid-%d", i % 3);
        CU_ASSERT(strcmp(device->uuid, select_device[i].uuid) == 0);
        sprintf(device->name,"name-%d", i % 3);
        CU_ASSERT(strcmp(device->name, select_device[i].name) == 0);
        sprintf(device->password, "passwd-%d", i % 3);
        CU_ASSERT(strcmp(device->password, select_device[i].password) == 0);

        CU_ASSERT(select_volume[i].id == i);
        CU_ASSERT(select_volume[i].device_id == i % 3);
        sprintf(volume->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(volume->uuid, select_volume[i].uuid) == 0);
        sprintf(volume->drive,"drive-%d", i);
        CU_ASSERT(strcmp(volume->drive, select_volume[i].drive) == 0);
        sprintf(volume->label, "label-%d", i);
        CU_ASSERT(strcmp(volume->label, select_volume[i].label) == 0);
    }
    device_free(NULL, select_device);
    volume_free(NULL, select_volume);

    // //printf("test left join");
    ret = sorm_select_all_array_by_join(conn, NULL,  ALL_COLUMNS, DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_LEFT_JOIN, NULL, &n,
            (sorm_table_descriptor_t**)&select_device,(sorm_table_descriptor_t**) &select_volume);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == 6);
    for(i = 0; i < n; i ++)
    {
        CU_ASSERT(select_device[i].id == device_index[i]);
        sprintf(device->uuid,"uuid-%d", device_index[i]);
        CU_ASSERT(strcmp(device->uuid, select_device[i].uuid) == 0);
        sprintf(device->name,"name-%d", device_index[i]);
        CU_ASSERT(strcmp(device->name, select_device[i].name) == 0);
        sprintf(device->password, "passwd-%d", device_index[i]);
        CU_ASSERT(strcmp(device->password, select_device[i].password) == 0);

        if(i != (n - 1)) 
        { 
            CU_ASSERT(select_volume[i].id == volume_index[i]); 
            CU_ASSERT(select_volume[i].device_id == device_index[i]);
            sprintf(volume->uuid,"uuid-%d", volume_index[i]);
            CU_ASSERT(strcmp(volume->uuid, select_volume[i].uuid) == 0);
            sprintf(volume->drive,"drive-%d", volume_index[i]);
            CU_ASSERT(strcmp(volume->drive, select_volume[i].drive) == 0);
            sprintf(volume->label, "label-%d", volume_index[i]);
            CU_ASSERT(strcmp(volume->label, select_volume[i].label) == 0);
        }
    }
    device_free(NULL, select_device);
    volume_free(NULL, select_volume);

    // //printf("select by fileter\n");
    ret = sorm_select_all_array_by_join(conn, NULL,  ALL_COLUMNS, 
            DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, COLUMN__DEVICE__ID" = 0",
            &n, (sorm_table_descriptor_t**)&select_device, 
            (sorm_table_descriptor_t**)&select_volume);

    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == 2);
    device_index[0] = 0;
    device_index[1] = 0;
    volume_index[0] = 0;
    volume_index[1] = 3;
    for(i = 0; i < n; i ++)
    {
        CU_ASSERT(select_device[i].id == device_index[i]);
        sprintf(device->uuid,"uuid-%d", device_index[i]);
        CU_ASSERT(strcmp(device->uuid, select_device[i].uuid) == 0);
        sprintf(device->name,"name-%d", device_index[i]);
        CU_ASSERT(strcmp(device->name, select_device[i].name) == 0);
        sprintf(device->password, "passwd-%d", device_index[i]);
        CU_ASSERT(strcmp(device->password, select_device[i].password) == 0);

        CU_ASSERT(select_volume[i].id == volume_index[i]); 
        CU_ASSERT(select_volume[i].device_id == device_index[i]);
        sprintf(volume->uuid,"uuid-%d", volume_index[i]);
        CU_ASSERT(strcmp(volume->uuid, select_volume[i].uuid) == 0);
        sprintf(volume->drive,"drive-%d", volume_index[i]);
        CU_ASSERT(strcmp(volume->drive, select_volume[i].drive) == 0);
        sprintf(volume->label, "label-%d", volume_index[i]);
        CU_ASSERT(strcmp(volume->label, select_volume[i].label) == 0);
    }
    device_free(NULL, select_device);
    volume_free(NULL, select_volume);

    // //printf("test select some by join\n");
    n = 3;
    ret = sorm_select_some_array_by_join(conn, NULL,  ALL_COLUMNS, DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, NULL, &n,
            (sorm_table_descriptor_t**)&select_device, (sorm_table_descriptor_t**)&select_volume);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == 3);
    for(i = 0; i < n; i ++)
    {
        CU_ASSERT(select_device[i].id == i % 3);
        sprintf(device->uuid,"uuid-%d", i % 3);
        CU_ASSERT(strcmp(device->uuid, select_device[i].uuid) == 0);
        sprintf(device->name,"name-%d", i % 3);
        CU_ASSERT(strcmp(device->name, select_device[i].name) == 0);
        sprintf(device->password, "passwd-%d", i % 3);
        CU_ASSERT(strcmp(device->password, select_device[i].password) == 0);

        CU_ASSERT(select_volume[i].id == i);
        CU_ASSERT(select_volume[i].device_id == i % 3);
        sprintf(volume->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(volume->uuid, select_volume[i].uuid) == 0);
        sprintf(volume->drive,"drive-%d", i);
        CU_ASSERT(strcmp(volume->drive, select_volume[i].drive) == 0);
        sprintf(volume->label, "label-%d", i);
        CU_ASSERT(strcmp(volume->label, select_volume[i].label) == 0);
    }
    device_free(NULL, select_device);
    volume_free(NULL, select_volume);
    n = 4;
    ret = sorm_select_some_array_by_join(conn, NULL,  ALL_COLUMNS, DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, NULL, &n,
            (sorm_table_descriptor_t**)&select_device, (sorm_table_descriptor_t**)&select_volume);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == 4);
    for(i = 0; i < n; i ++)
    {
        CU_ASSERT(select_device[i].id == i % 3);
        sprintf(device->uuid,"uuid-%d", i % 3);
        CU_ASSERT(strcmp(device->uuid, select_device[i].uuid) == 0);
        sprintf(device->name,"name-%d", i % 3);
        CU_ASSERT(strcmp(device->name, select_device[i].name) == 0);
        sprintf(device->password, "passwd-%d", i % 3);
        CU_ASSERT(strcmp(device->password, select_device[i].password) == 0);

        CU_ASSERT(select_volume[i].id == i);
        CU_ASSERT(select_volume[i].device_id == i % 3);
        sprintf(volume->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(volume->uuid, select_volume[i].uuid) == 0);
        sprintf(volume->drive,"drive-%d", i);
        CU_ASSERT(strcmp(volume->drive, select_volume[i].drive) == 0);
        sprintf(volume->label, "label-%d", i);
        CU_ASSERT(strcmp(volume->label, select_volume[i].label) == 0);
    }
    device_free(NULL, select_device);
    volume_free(NULL, select_volume);

    n = 0;
    ret = sorm_select_some_array_by_join(conn, NULL,  ALL_COLUMNS, DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, NULL, &n,
            (sorm_table_descriptor_t**)&select_device, (sorm_table_descriptor_t**)&select_volume);
    CU_ASSERT(ret == SORM_NOEXIST);
    CU_ASSERT(n == 0);
    CU_ASSERT(select_device == NULL);
    CU_ASSERT(select_volume == NULL);
    
    ret = sorm_select_some_array_by_join(conn, NULL,  ALL_COLUMNS, DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, NULL, &n,
            (sorm_table_descriptor_t**)&select_device, (sorm_table_descriptor_t**)&select_volume);

    /* test select iterator */
    sorm_iterator_t *iterator;
    i = 0;
    ret = sorm_select_iterate_by_join_open(conn, NULL, ALL_COLUMNS,
            DESC__DEVICE, COLUMN__DEVICE__ID, 
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, 
            SORM_INNER_JOIN, NULL, &iterator);
    assert(iterator != NULL);
    CU_ASSERT(ret == SORM_OK);
    while(1) {
        sorm_select_iterate_by_join(iterator, (sorm_table_descriptor_t**)&select_device, 
                (sorm_table_descriptor_t**)&select_volume);

        if (!sorm_select_iterate_more(iterator)) {
            break;
        }
    
        char string[1024];
        device_to_string(select_device, string, 1024);
        log_debug("select : %s", string);
        volume_to_string(select_volume, string, 1024);
        log_debug("select : %s", string);
        
        CU_ASSERT(select_device->id == i % 3);
        sprintf(device->uuid,"uuid-%d", i % 3);
        CU_ASSERT(strcmp(device->uuid, select_device->uuid) == 0);
        sprintf(device->name,"name-%d", i % 3);
        CU_ASSERT(strcmp(device->name, select_device->name) == 0);
        sprintf(device->password, "passwd-%d", i % 3);
        CU_ASSERT(strcmp(device->password, select_device->password) == 0);

        CU_ASSERT(select_volume->id == i);
        CU_ASSERT(select_volume->device_id == i % 3);
        sprintf(volume->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(volume->uuid, select_volume->uuid) == 0);
        sprintf(volume->drive,"drive-%d", i);
        CU_ASSERT(strcmp(volume->drive, select_volume->drive) == 0);
        sprintf(volume->label, "label-%d", i);
        CU_ASSERT(strcmp(volume->label, select_volume->label) == 0);
        device_free(NULL, select_device);
        volume_free(NULL, select_volume);
        i ++;
    }
    CU_ASSERT(i == 5);
    sorm_select_iterate_close(iterator);
    device_free(NULL, device);
    volume_free(NULL, volume);

    for(i = 0; i < 3; i ++)
    {
        device_delete_by_id(conn, i);
    }
    for(i = 0; i < 5; i ++)
    {
        volume_delete_by_id(conn, i);
    }

    /* select from empty table */
    n = 4;
    ret = sorm_select_some_array_by_join(conn, NULL,  ALL_COLUMNS, DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, NULL, &n,
            (sorm_table_descriptor_t**)&select_device, (sorm_table_descriptor_t**)&select_volume);
    CU_ASSERT(ret == SORM_NOEXIST);
    CU_ASSERT(n == 0);
    CU_ASSERT(select_device == NULL);
    CU_ASSERT(select_volume == NULL);

    ret = sorm_select_all_array_by_join(conn, NULL,  ALL_COLUMNS, DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, NULL, &n,
            (sorm_table_descriptor_t**)&select_device, (sorm_table_descriptor_t**)&select_volume);
    CU_ASSERT(ret == SORM_NOEXIST);
    CU_ASSERT(n == 0);
    CU_ASSERT(select_device == NULL);
    CU_ASSERT(select_volume == NULL);

    sorm_list_t *list1, *list2;
    ret = sorm_select_all_list_by_join(conn, NULL,  ALL_COLUMNS, DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, NULL, &n,
            &list1, &list2);
    CU_ASSERT(ret == SORM_NOEXIST);
    CU_ASSERT(n == 0);
    CU_ASSERT(select_device == NULL);
    CU_ASSERT(select_volume == NULL);
}

static void test_sorm_select_columns_by_join()
{
    device_t *device;
    volume_t *volume;
    int i, n, ret;
    device_t *select_device;
    volume_t *select_volume;

    /* insert rows for select */
    device = device_new(NULL);
    for(i = 0; i < 4; i ++)
    {
        device->id = i;
        device->id_stat = SORM_STAT_VALUED;
        sprintf(device->uuid,"uuid-%d", i);
        device->uuid_stat = SORM_STAT_VALUED;
        sprintf(device->name,"name-%d", i);
        device->name_stat = SORM_STAT_VALUED;
        sprintf(device->password, "passwd-%d", i);
        device->password_stat = SORM_STAT_VALUED;
        device_insert(conn, device);
    }

    volume = volume_new(NULL);
    for(i = 0; i < 5; i ++)
    {
        volume->id = i;
        volume->id_stat = SORM_STAT_VALUED;
        volume->device_id = i % 3;
        volume->device_id_stat = SORM_STAT_VALUED;
        sprintf(volume->uuid, "uuid-%d", i);
        volume->uuid_stat = SORM_STAT_VALUED;
        sprintf(volume->drive, "drive-%d", i);
        volume->drive_stat = SORM_STAT_VALUED;
        sprintf(volume->label, "label-%d", i);
        volume->label_stat = SORM_STAT_VALUED;
        volume_insert(conn, volume);
    }
    i = 6;
    volume->id = i;
    volume->id_stat = SORM_STAT_VALUED;
    volume->device_id = 100;
    volume->device_id_stat = SORM_STAT_VALUED;
    sprintf(volume->uuid, "uuid-%d", i);
    volume->uuid_stat = SORM_STAT_VALUED;
    sprintf(volume->drive, "drive-%d", i);
    volume->uuid_stat = SORM_STAT_VALUED;
    sprintf(volume->label, "label-%d", i);
    volume->label_stat = SORM_STAT_VALUED;
    volume_insert(conn, volume);

    //printf("select device.id and volume.id\n");
    ret = sorm_select_all_array_by_join(conn, NULL,  
            "device.id, volume.id"
            , DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, NULL, &n,
            (sorm_table_descriptor_t**)&select_device, 
            (sorm_table_descriptor_t**)&select_volume);

    CU_ASSERT(n == 5);
    for(i = 0; i < n; i ++)
    {
        CU_ASSERT(select_device[i].id == i % 3);

        CU_ASSERT(select_volume[i].id == i);
    }
    device_free(NULL, select_device);
    volume_free(NULL, select_volume);

    //printf("select * and volume.id\n");
    ret = sorm_select_all_array_by_join(conn, NULL,  
            "*, volume.id"
            , DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, NULL, &n,
            (sorm_table_descriptor_t**)&select_device, 
            (sorm_table_descriptor_t**)&select_volume);

    CU_ASSERT(n == 5);
    for(i = 0; i < n; i ++)
    {
        CU_ASSERT(select_device[i].id == i % 3);
        sprintf(device->uuid,"uuid-%d", i % 3);
        CU_ASSERT(strcmp(device->uuid, select_device[i].uuid) == 0);
        sprintf(device->name,"name-%d", i % 3);
        CU_ASSERT(strcmp(device->name, select_device[i].name) == 0);
        sprintf(device->password, "passwd-%d", i % 3);
        CU_ASSERT(strcmp(device->password, select_device[i].password) == 0);

        CU_ASSERT(select_volume[i].id == i);
        CU_ASSERT(select_volume[i].device_id == i % 3);
        sprintf(volume->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(volume->uuid, select_volume[i].uuid) == 0);
        sprintf(volume->drive,"drive-%d", i);
        CU_ASSERT(strcmp(volume->drive, select_volume[i].drive) == 0);
        sprintf(volume->label, "label-%d", i);
        CU_ASSERT(strcmp(volume->label, select_volume[i].label) == 0);
    }
    device_free(NULL, select_device);
    volume_free(NULL, select_volume);

    //printf("select device.id and *\n");
    ret = sorm_select_all_array_by_join(conn, NULL,  
            "device.id, *"
            , DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, NULL, &n,
            (sorm_table_descriptor_t**)&select_device, 
            (sorm_table_descriptor_t**)&select_volume);

    CU_ASSERT(n == 5);
    for(i = 0; i < n; i ++)
    {
        CU_ASSERT(select_device[i].id == i % 3);
        sprintf(device->uuid,"uuid-%d", i % 3);
        CU_ASSERT(strcmp(device->uuid, select_device[i].uuid) == 0);
        sprintf(device->name,"name-%d", i % 3);
        CU_ASSERT(strcmp(device->name, select_device[i].name) == 0);
        sprintf(device->password, "passwd-%d", i % 3);
        CU_ASSERT(strcmp(device->password, select_device[i].password) == 0);

        CU_ASSERT(select_volume[i].id == i);
        CU_ASSERT(select_volume[i].device_id == i % 3);
        sprintf(volume->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(volume->uuid, select_volume[i].uuid) == 0);
        sprintf(volume->drive,"drive-%d", i);
        CU_ASSERT(strcmp(volume->drive, select_volume[i].drive) == 0);
        sprintf(volume->label, "label-%d", i);
        CU_ASSERT(strcmp(volume->label, select_volume[i].label) == 0);
    }
    device_free(NULL, select_device);
    volume_free(NULL, select_volume);

    //printf("select id\n");
    ret = sorm_select_all_array_by_join(conn, NULL,  
            "id", DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, NULL, &n,
            (sorm_table_descriptor_t**)&select_device, 
            (sorm_table_descriptor_t**)&select_volume);

    CU_ASSERT(ret == SORM_DB_ERROR);
    CU_ASSERT(select_device == NULL);
    CU_ASSERT(select_volume == NULL);

    //printf("select drive\n");
    ret = sorm_select_all_array_by_join(conn, NULL,  
            "drive"
            , DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, NULL, &n,
            (sorm_table_descriptor_t**)&select_device, 
            (sorm_table_descriptor_t**)&select_volume);

    CU_ASSERT(n == 5);
    for(i = 0; i < n; i ++)
    {
        sprintf(volume->drive,"drive-%d", i);
        CU_ASSERT(strcmp(volume->drive, select_volume[i].drive) == 0);
    }
    device_free(NULL, select_device);
    volume_free(NULL, select_volume);

    //printf("select passwd\n");
    ret = sorm_select_all_array_by_join(conn, NULL,  
            "password"
            , DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, NULL, &n,
            (sorm_table_descriptor_t**)&select_device, 
            (sorm_table_descriptor_t**)&select_volume);

    CU_ASSERT(n == 5);
    for(i = 0; i < n; i ++)
    {
        sprintf(device->password, "passwd-%d", i % 3);
        CU_ASSERT(strcmp(device->password, select_device[i].password) == 0);
    }
    device_free(NULL, select_device);
    volume_free(NULL, select_volume);

    //printf("select drive, drive, passwd, drive\n");
    ret = sorm_select_all_array_by_join(conn, NULL,  
            "drive, drive, password, drive"
            , DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, NULL, &n,
            (sorm_table_descriptor_t**)&select_device, 
            (sorm_table_descriptor_t**)&select_volume);

    CU_ASSERT(n == 5);
    for(i = 0; i < n; i ++)
    {
        sprintf(volume->drive,"drive-%d", i);
        CU_ASSERT(strcmp(volume->drive, select_volume[i].drive) == 0);

        sprintf(device->password, "passwd-%d", i % 3);
        CU_ASSERT(strcmp(device->password, select_device[i].password) == 0);
    }
    device_free(NULL, select_device);
    volume_free(NULL, select_volume);

    //printf("select passwd, drive\n");
    ret = sorm_select_all_array_by_join(conn, NULL,  
            "password, drive"
            , DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, NULL, &n,
            (sorm_table_descriptor_t**)&select_device, 
            (sorm_table_descriptor_t**)&select_volume);

    CU_ASSERT(n == 5);
    for(i = 0; i < n; i ++)
    {
        sprintf(volume->drive,"drive-%d", i);
        CU_ASSERT(strcmp(volume->drive, select_volume[i].drive) == 0);

        sprintf(device->password, "passwd-%d", i % 3);
        CU_ASSERT(strcmp(device->password, select_device[i].password) == 0);
    }
    device_free(NULL, select_device);
    volume_free(NULL, select_volume);

    //printf("select drive, passwd\n");
    ret = sorm_select_all_array_by_join(conn, NULL,  
            "drive, password"
            , DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, NULL, &n,
            (sorm_table_descriptor_t**)&select_device, 
            (sorm_table_descriptor_t**)&select_volume);

    CU_ASSERT(n == 5);
    for(i = 0; i < n; i ++)
    {
        sprintf(volume->drive,"drive-%d", i);
        CU_ASSERT(strcmp(volume->drive, select_volume[i].drive) == 0);

        sprintf(device->password, "passwd-%d", i % 3);
        CU_ASSERT(strcmp(device->password, select_device[i].password) == 0);
    }
    device_free(NULL, select_device);
    volume_free(NULL, select_volume);

    //printf("select device.* and volume.uuid\n");
    ret = sorm_select_all_array_by_join(conn, NULL,  
            "device.*, volume.uuid"
            , DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, NULL, &n,
            (sorm_table_descriptor_t**)&select_device, 
            (sorm_table_descriptor_t**)&select_volume);

    CU_ASSERT(n == 5);
    for(i = 0; i < n; i ++)
    {
        CU_ASSERT(select_device[i].id == i % 3);
        sprintf(device->uuid,"uuid-%d", i % 3);
        CU_ASSERT(strcmp(device->uuid, select_device[i].uuid) == 0);
        sprintf(device->name,"name-%d", i % 3);
        CU_ASSERT(strcmp(device->name, select_device[i].name) == 0);
        sprintf(device->password, "passwd-%d", i % 3);
        CU_ASSERT(strcmp(device->password, select_device[i].password) == 0);

        sprintf(volume->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(volume->uuid, select_volume[i].uuid) == 0);
    }
    device_free(NULL, select_device);
    volume_free(NULL, select_volume);

    //printf("select device.uuid and volume.*\n");
    ret = sorm_select_all_array_by_join(conn, NULL,  
            "device.uuid, volume.*"
            , DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, NULL, &n,
            (sorm_table_descriptor_t**)&select_device, 
            (sorm_table_descriptor_t**)&select_volume);

    CU_ASSERT(n == 5);
    for(i = 0; i < n; i ++)
    {
        sprintf(device->uuid,"uuid-%d", i % 3);
        CU_ASSERT(strcmp(device->uuid, select_device[i].uuid) == 0);

        CU_ASSERT(select_volume[i].id == i);
        CU_ASSERT(select_volume[i].device_id == i % 3);
        sprintf(volume->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(volume->uuid, select_volume[i].uuid) == 0);
        sprintf(volume->drive,"drive-%d", i);
        CU_ASSERT(strcmp(volume->drive, select_volume[i].drive) == 0);
        sprintf(volume->label, "label-%d", i);
        CU_ASSERT(strcmp(volume->label, select_volume[i].label) == 0);
    }
    device_free(NULL, select_device);
    volume_free(NULL, select_volume);

    for(i = 0; i < 4; i ++)
    {
        device_delete_by_id(conn, i);
    }
    for(i = 0; i < 5; i ++)
    {
        volume_delete_by_id(conn, i);
    }

    device_free(NULL, device);
    volume_free(NULL, volume);
}
static void test_volume_select_by_driver(void)
{
    device_t *device;
    volume_t *volume;
    int i, n, ret;
    device_t *select_device;
    volume_t *select_volume;
    sorm_list_t *list;
    int device_index[6]={0, 0, 1, 1, 2, 3};
    int volume_index[6]={0, 3, 1, 4, 2, 0};

    /* insert rows for select */
    device = device_new(NULL);
    for(i = 0; i < 4; i ++)
    {
        device->id = i;
        device->id_stat = SORM_STAT_VALUED;
        sprintf(device->uuid,"uuid-%d", i);
        device->uuid_stat = SORM_STAT_VALUED;
        sprintf(device->name,"name-%d", i);
        device->name_stat = SORM_STAT_VALUED;
        sprintf(device->password, "passwd-%d", i);
        device->password_stat = SORM_STAT_VALUED;
        device_insert(conn, device);
    }

    volume = volume_new(NULL);
    for(i = 0; i < 5; i ++)
    {
        volume->id = i;
        volume->id_stat = SORM_STAT_VALUED;
        volume->device_id = i % 3;
        volume->device_id_stat = SORM_STAT_VALUED;
        sprintf(volume->uuid, "uuid-%d", i);
        volume->uuid_stat = SORM_STAT_VALUED;
        sprintf(volume->drive, "drive-%d", i);
        volume->drive_stat = SORM_STAT_VALUED;
        sprintf(volume->label, "label-%d", i);
        volume->label_stat = SORM_STAT_VALUED;
        volume_insert(conn, volume);
    }
    i = 6;
    volume->id = i;
    volume->id_stat = SORM_STAT_VALUED;
    volume->device_id = 100;
    volume->device_id_stat = SORM_STAT_VALUED;
    sprintf(volume->uuid, "uuid-%d", i);
    volume->uuid_stat = SORM_STAT_VALUED;
    sprintf(volume->drive, "drive-%d", i);
    volume->uuid_stat = SORM_STAT_VALUED;
    sprintf(volume->label, "label-%d", i);
    volume->label_stat = SORM_STAT_VALUED;
    volume_insert(conn, volume);

    ret = volume_select_all_array_by_device(conn, NULL, "volume.*", 
            NULL, &n, &select_volume);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == 5);
    for(i = 0; i < n; i ++)
    {
        CU_ASSERT(select_volume[i].id == i);
        CU_ASSERT(select_volume[i].device_id == i % 3);
        sprintf(volume->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(volume->uuid, select_volume[i].uuid) == 0);
        sprintf(volume->drive,"drive-%d", i);
        CU_ASSERT(strcmp(volume->drive, select_volume[i].drive) == 0);
        sprintf(volume->label, "label-%d", i);
        CU_ASSERT(strcmp(volume->label, select_volume[i].label) == 0);
    }
    volume_free(NULL, select_volume);

    n = 3;
    ret = volume_select_some_array_by_device(conn, NULL, "volume.*", 
            NULL, &n, &select_volume);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == 3);
    for(i = 0; i < n; i ++)
    {
        CU_ASSERT(select_volume[i].id == i);
        CU_ASSERT(select_volume[i].device_id == i % 3);
        sprintf(volume->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(volume->uuid, select_volume[i].uuid) == 0);
        sprintf(volume->drive,"drive-%d", i);
        CU_ASSERT(strcmp(volume->drive, select_volume[i].drive) == 0);
        sprintf(volume->label, "label-%d", i);
        CU_ASSERT(strcmp(volume->label, select_volume[i].label) == 0);
    }
    volume_free(NULL, select_volume);

    /*  use filter */
    ret = volume_select_all_array_by_device(conn, NULL, TABLE__VOLUME".*", 
            COLUMN__DEVICE__ID" = 0", &n, &select_volume);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == 2);
    for(i = 0; i < n; i ++)
    {
        CU_ASSERT(select_volume[i].id == volume_index[i]);
        CU_ASSERT(select_volume[i].device_id == 0);
        sprintf(volume->uuid,"uuid-%d", volume_index[i]);
        CU_ASSERT(strcmp(volume->uuid, select_volume[i].uuid) == 0);
        sprintf(volume->drive,"drive-%d", volume_index[i]);
        CU_ASSERT(strcmp(volume->drive, select_volume[i].drive) == 0);
        sprintf(volume->label, "label-%d", volume_index[i]);
        CU_ASSERT(strcmp(volume->label, select_volume[i].label) == 0);
    }
    volume_free(NULL, select_volume);

    for(i = 0; i < 3; i ++)
    {
        device_delete_by_id(conn, i);
    }
    for(i = 0; i < 5; i ++)
    {
        volume_delete_by_id(conn, i);
    }
    device_free(NULL, device);
    volume_free(NULL, volume);


    //printf("select invalid column \n");
    ret = volume_select_some_array_by(conn, NULL,  "invalid", NULL, &n, &select_volume);
    CU_ASSERT(ret == SORM_DB_ERROR);
    CU_ASSERT(select_volume == NULL);
    volume_free(NULL, select_volume);
    ret = volume_select_all_array_by(conn, NULL,  "invalid", NULL, &n, &select_volume);
    CU_ASSERT(ret == SORM_DB_ERROR);
    CU_ASSERT(select_volume == NULL);
    volume_free(NULL, select_volume);
    ret = volume_select_some_list_by(conn, NULL,  "invalid", NULL, &n, &list);
    CU_ASSERT(ret == SORM_DB_ERROR);
    CU_ASSERT(list == NULL);
    volume_free(NULL, select_volume);
    ret = volume_select_all_list_by(conn, NULL,  "invalid", NULL, &n, &list);
    CU_ASSERT(ret == SORM_DB_ERROR);
    CU_ASSERT(list == NULL);
    volume_free(NULL, select_volume);
}

static void test_sorm_select_null_by_join(void)
{
    device_t *device;
    volume_t *volume;
    int i, n, ret;
    device_t *select_device;
    volume_t *select_volume;
    int device_index[6]={0, 0, 1, 1, 2, 3};
    int volume_index[6]={0, 3, 1, 4, 2, 0};

    /* insert rows for select */
    device = device_new(NULL);
    for(i = 0; i < 4; i ++)
    {
        device->id = i;
        device->id_stat = SORM_STAT_VALUED;
        sprintf(device->uuid,"uuid-%d", i);
        device->uuid_stat = SORM_STAT_VALUED;
        sprintf(device->name,"name-%d", i);
        device->name_stat = SORM_STAT_VALUED;
        sprintf(device->password, "passwd-%d", i);
        device->password_stat = SORM_STAT_VALUED;
        device_insert(conn, device);
    }

    volume = volume_new(NULL);
    for(i = 0; i < 5; i ++)
    {
        volume->id = i;
        volume->id_stat = SORM_STAT_VALUED;
        volume->device_id = i % 3;
        volume->device_id_stat = SORM_STAT_VALUED;
        sprintf(volume->uuid, "uuid-%d", i);
        volume->uuid_stat = SORM_STAT_VALUED;
        sprintf(volume->drive, "drive-%d", i);
        volume->drive_stat = SORM_STAT_VALUED;
        sprintf(volume->label, "label-%d", i);
        volume->label_stat = SORM_STAT_VALUED;
        volume_insert(conn, volume);
    }
    i = 6;
    volume->id = i;
    volume->id_stat = SORM_STAT_VALUED;
    volume->device_id = 100;
    volume->device_id_stat = SORM_STAT_VALUED;
    sprintf(volume->uuid, "uuid-%d", i);
    volume->uuid_stat = SORM_STAT_VALUED;
    sprintf(volume->drive, "drive-%d", i);
    volume->uuid_stat = SORM_STAT_VALUED;
    sprintf(volume->label, "label-%d", i);
    volume->label_stat = SORM_STAT_VALUED;
    volume_insert(conn, volume);

    ret = sorm_select_all_array_by_join(conn, NULL,  ALL_COLUMNS, DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, NULL, &n,
            NULL, (sorm_table_descriptor_t**)&select_volume);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == 5);
    for(i = 0; i < n; i ++)
    {
        CU_ASSERT(select_volume[i].id == i);
        CU_ASSERT(select_volume[i].device_id == i % 3);
        sprintf(volume->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(volume->uuid, select_volume[i].uuid) == 0);
        sprintf(volume->drive,"drive-%d", i);
        CU_ASSERT(strcmp(volume->drive, select_volume[i].drive) == 0);
        sprintf(volume->label, "label-%d", i);
        CU_ASSERT(strcmp(volume->label, select_volume[i].label) == 0);
    }
    volume_free(NULL, select_volume);

    ret = sorm_select_all_array_by_join(conn, NULL,  ALL_COLUMNS, DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, NULL, &n,
            (sorm_table_descriptor_t**)&select_device, NULL);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == 5);
    for(i = 0; i < n; i ++)
    {
        CU_ASSERT(select_device[i].id == i % 3);
        sprintf(device->uuid,"uuid-%d", i % 3);
        CU_ASSERT(strcmp(device->uuid, select_device[i].uuid) == 0);
        sprintf(device->name,"name-%d", i % 3);
        CU_ASSERT(strcmp(device->name, select_device[i].name) == 0);
        sprintf(device->password, "passwd-%d", i % 3);
        CU_ASSERT(strcmp(device->password, select_device[i].password) == 0);
    }
    device_free(NULL, select_device);

    ret = sorm_select_all_array_by_join(conn, NULL,  ALL_COLUMNS, DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, NULL, &n,
            NULL, NULL);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == 5);

    n = 3;
    ret = sorm_select_some_array_by_join(conn, NULL,  ALL_COLUMNS, DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, NULL, &n,
            NULL, (sorm_table_descriptor_t**)&select_volume);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == 3);
    for(i = 0; i < n; i ++)
    {
        CU_ASSERT(select_volume[i].id == i);
        CU_ASSERT(select_volume[i].device_id == i % 3);
        sprintf(volume->uuid,"uuid-%d", i);
        CU_ASSERT(strcmp(volume->uuid, select_volume[i].uuid) == 0);
        sprintf(volume->drive,"drive-%d", i);
        CU_ASSERT(strcmp(volume->drive, select_volume[i].drive) == 0);
        sprintf(volume->label, "label-%d", i);
        CU_ASSERT(strcmp(volume->label, select_volume[i].label) == 0);
    }
    volume_free(NULL, select_volume);

    ret = sorm_select_some_array_by_join(conn, NULL,  ALL_COLUMNS, DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, NULL, &n,
            (sorm_table_descriptor_t**)&select_device, NULL);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == 3);
    for(i = 0; i < n; i ++)
    {
        CU_ASSERT(select_device[i].id == i % 3);
        sprintf(device->uuid,"uuid-%d", i % 3);
        CU_ASSERT(strcmp(device->uuid, select_device[i].uuid) == 0);
        sprintf(device->name,"name-%d", i % 3);
        CU_ASSERT(strcmp(device->name, select_device[i].name) == 0);
        sprintf(device->password, "passwd-%d", i % 3);
        CU_ASSERT(strcmp(device->password, select_device[i].password) == 0);
    }
    device_free(NULL, select_device);

    ret = sorm_select_some_array_by_join(conn, NULL,  ALL_COLUMNS, DESC__DEVICE, COLUMN__DEVICE__ID,
            DESC__VOLUME, COLUMN__VOLUME__DEVICE_ID, SORM_INNER_JOIN, NULL, &n,
            NULL, NULL);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == 3);

    for(i = 0; i < 3; i ++)
    {
        device_delete_by_id(conn, i);
    }
    for(i = 0; i < 5; i ++)
    {
        volume_delete_by_id(conn, i);
    }
    device_free(NULL, device);
    volume_free(NULL, volume);

}

static void test_sorm_strerror(void)
{
    CU_ASSERT(strcmp(sorm_strerror(SORM_OK), "There is no error") == 0);
    CU_ASSERT(strcmp(sorm_strerror(SORM_INVALID_NUM), 
                "The number of rows to be selected is invalid") == 0); 

    CU_ASSERT(strcmp(sorm_strerror(SORM_ARG_NULL), 
                "One or more arguments is NULL") == 0);

}

static void test_sorm_unique(void)
{
    device_t *device, *select_device;
    int ret;

    device = device_new(NULL);

    device->id = 1;
    device->id_stat = SORM_STAT_VALUED;
    sprintf(device->uuid,"uuid");
    device->uuid_stat = SORM_STAT_VALUED;
    sprintf(device->name,"name");
    device->name_stat = SORM_STAT_VALUED;
    sprintf(device->password, "passwd");
    device->password_stat = SORM_STAT_VALUED;
    device_insert(conn, device);
    device->id = 2;
    device->id_stat = SORM_STAT_VALUED;
    ret = device_insert(conn, device);

    ret = device_select_by_id(conn, NULL, ALL_COLUMNS, 1, &select_device);
    CU_ASSERT(ret == SORM_NOEXIST);
    CU_ASSERT(select_device == NULL);
    ret = device_select_by_id(conn, NULL, ALL_COLUMNS, 2, &select_device);
    CU_ASSERT(ret == SORM_OK);
    device_free(NULL, select_device);

    ret = device_delete_by_id(conn, 1);
    ret = device_delete_by_id(conn, 2);

    device->id = 1;
    device->id_stat = SORM_STAT_VALUED;
    sprintf(device->uuid,"uuid");
    device->uuid_stat = SORM_STAT_VALUED;
    sprintf(device->name,"name");
    device->name_stat = SORM_STAT_VALUED;
    sprintf(device->password, "passwd");
    device->password_stat = SORM_STAT_VALUED;
    device_insert(conn, device);
    device->id = 2;
    device->id_stat = SORM_STAT_VALUED;
    sprintf(device->uuid,"uuid1");
    device->uuid_stat = SORM_STAT_VALUED;
    ret = device_insert(conn, device);

    ret = device_select_by_id(conn, NULL, ALL_COLUMNS, 1, &select_device);
    CU_ASSERT(ret == SORM_OK);
    device_free(NULL, select_device);
    ret = device_select_by_id(conn, NULL, ALL_COLUMNS, 2, &select_device);
    CU_ASSERT(ret == SORM_OK);
    device_free(NULL, select_device);

    ret = device_delete_by_id(conn, 1);
    ret = device_delete_by_id(conn, 2);

    device_free(NULL, device);
}

static void test_sorm_foreign_key(void)
{
    device_t *device;
    volume_t *volume;
    int ret, i;
    for(i = 0; i < 2; i ++)
    {
        device = device_new(NULL);

        device->id = i;
        device->id_stat = SORM_STAT_VALUED;
        sprintf(device->uuid,"uuid-%d", i);
        device->uuid_stat = SORM_STAT_VALUED;
        sprintf(device->name,"name-%d", i);
        device->name_stat = SORM_STAT_VALUED;
        sprintf(device->password, "passwd-%d", i);
        device->password_stat = SORM_STAT_VALUED;

        device_insert(conn, device);

        device_free(NULL, device);
    }


    volume = volume_new(NULL);
    volume_set_device_id(volume, 4);
    volume_set_uuid(volume, "aaaa");
    volume_set_drive(volume, "aaaa");
    volume_set_label(volume, "aaaa");

    ret = volume_insert(conn, volume);
    CU_ASSERT(ret == SORM_DB_ERROR);
    volume_free(NULL, volume);

    volume = volume_new(NULL);
    volume_set_device_id(volume, 1);
    volume_set_uuid(volume, "aaaa");
    volume_set_drive(volume, "aaaa");
    volume_set_label(volume, "aaaa");

    ret = volume_insert(conn, volume);
    CU_ASSERT(ret == SORM_OK);
    volume_free(NULL, volume);
}

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
    
    device_t *device = device_new(NULL);
    device_set_uuid(device, "uuid0");
    device_set_small_num(device, 10);
    ret = device_insert(_conn, device);
    assert(ret == SORM_OK);

    sorm_close(_conn);
}

static void* pthread_c_work(void* args) {
    sorm_connection_t *_conn;
    int ret = sorm_open(DB_FILE, SORM_DB_SQLITE, 0, &rwlock, 
            SORM_ENABLE_RWLOCK, &_conn);
    assert(ret == SORM_OK);
    
    pthread_mutex_lock(&mutex);
    while (condition_a == 0) {
        pthread_cond_wait(&cond_a, &mutex); 
    }
    pthread_mutex_unlock(&mutex);
    
    device_t *device;
    ret = device_select_by_uuid(_conn, NULL, ALL_COLUMNS, "uuid0",
            &device);
    assert(ret == SORM_OK);
    device_free(NULL, device);
    
    CU_ASSERT(device->small_num == 2);

    sorm_close(_conn);
}

static void* pthread_b_work(void* args) {
    sorm_connection_t *_conn;
    int ret = sorm_open(DB_FILE, SORM_DB_SQLITE, 0, &rwlock, 
            SORM_ENABLE_RWLOCK, &_conn);
    assert(ret == SORM_OK);
    
    sorm_begin_write_transaction(_conn);

    device_t *device;
    ret = device_select_by_uuid(_conn, NULL, ALL_COLUMNS, "uuid0",
            &device);
    assert(ret == SORM_OK);
    
    pthread_mutex_lock(&mutex);
    condition_a = 1;
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&cond_a);

    printf("wait for input\n");
    ret = getchar();
    printf("continue running\n");

    device->small_num ++;
    ret = device_insert(_conn, device);
    assert(ret == SORM_OK);
    device_free(NULL, device);
    
    ret = device_select_by_uuid(_conn, NULL, ALL_COLUMNS, "uuid0",
            &device);
    assert(ret == SORM_OK);
    CU_ASSERT(device->small_num == 2);
    device_free(NULL, device);
    
    sorm_commit_transaction(_conn);

    sorm_close(_conn);
}

static void test_pthread_transaction(void) {
    pthread_rwlock_init(&rwlock, NULL);
    pthread_t p1, p2;
    int ret;
    
    device_t *device = device_new(NULL);
    device_set_uuid(device, "uuid0");
    device_set_small_num(device, 1);
    ret = device_insert(conn, device);
    assert(ret == SORM_OK);
    device_free(NULL, device);
    
    ret = pthread_create(&p1, NULL, pthread_a_work, (void*)conn);
    assert(ret == 0);
    ret = pthread_create(&p2, NULL, pthread_b_work, (void*)conn);
    assert(ret == 0);
    
    pthread_join(p1, NULL);
    pthread_join(p2, NULL);
    
    device = device_new(NULL);
    device_set_uuid(device, "uuid0");
    device_set_small_num(device, 1);
    ret = device_insert(conn, device);
    assert(ret == SORM_OK);
    device_free(NULL, device);
    
    ret = pthread_create(&p1, NULL, pthread_c_work, (void*)conn);
    assert(ret == 0);
    ret = pthread_create(&p2, NULL, pthread_b_work, (void*)conn);
    assert(ret == 0);
    
    pthread_join(p1, NULL);
    pthread_join(p2, NULL);

    device_delete_by_uuid(conn, "uuid0");
}

static void test_index(void)
{
    int ret;
    ret = device_create_index(conn, "uuid");
    CU_ASSERT(ret == SORM_OK);
    ret = device_drop_index(conn, "uuid");
    CU_ASSERT(ret == SORM_OK);
    ret = device_drop_index(conn, "uuid");
    CU_ASSERT(ret != SORM_OK);
    ret = device_drop_index(conn, "cid");
    CU_ASSERT(ret != SORM_OK);
}

static void test_create_drop_conflict(void)
{
    int ret; 

    ret = device_create_table(conn);
    CU_ASSERT(ret == SORM_OK);
    ret = device_delete_table(conn);
    CU_ASSERT(ret == SORM_OK);
    ret = device_create_table(conn);
    CU_ASSERT(ret == SORM_OK);
}

static void test_PK(void)
{
    device_t *device;
    int index = 0;
    int ret, i;
    char tmp[SQL_STMT_MAX_LEN];

    device = device_new(NULL);
    sprintf(tmp, "uuid-%d", index);
    device_set_uuid(device, tmp);
    ret = device_insert(conn, device);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(device->id == 1);
    device_free(NULL, device);

    for(i = 2; i <= 10; i ++)
    {
        device = device_new(NULL);
        sprintf(tmp, "uuid-%d", i);
        device_set_uuid(device, tmp);
        ret = device_insert(conn, device);
        CU_ASSERT(ret == SORM_OK);
        CU_ASSERT(device->id == i);
        device_free(NULL, device);
    }

    for(i = 0; i < 10; i ++)
    {
        ret = device_delete_by_id(conn, i);
    }
}

static void test_to_string(void)
{
    device_t *device;
    char string[1024];
    char string_short[4];
    int ret;
    char *ret_string;

    device = device_new(NULL);
    device_set_uuid(device, "uuid");

    char *string1 = "device { id(null); uuid(\"uuid\"); "
        "name(null); password(null); small_num(null); "
        "big_num(null); }";

    ret_string = device_to_string(device, string, 1024);
    CU_ASSERT(strcmp(string, string1) == 0);
    CU_ASSERT(strcmp(string, ret_string) == 0);
        
    device_set_id(device, 1);
    device_set_name(device, "name");
    device_set_password(device, "password");
    
    char *string2 = "device { "
        "id(1); "
        "uuid(\"uuid\"); "
        "name(\"name\"); "
        "password(\"password\"); "
        "small_num(null); "
        "big_num(null); }";
    ret_string = device_to_string(device, string, 1024);
    CU_ASSERT(strcmp(string, string2) == 0);
    CU_ASSERT(strcmp(string, ret_string) == 0);
    
    char *string3 = "dev";
    ret_string = device_to_string(device, string_short, 4);
    CU_ASSERT(strcmp(string_short, string3) == 0);
    CU_ASSERT(strcmp(string_short, ret_string) == 0);

    device_free(NULL, device);
}

static void test_text(void)
{
    text_blob_t *text_blob, *select_text_blob;
    int ret;

    text_blob = text_blob_new(NULL);
    CU_ASSERT(text_blob != NULL);

    ret = text_blob_set_id(text_blob, 1);
    ret = text_blob_set_text_heap(text_blob, "text_heap");
    CU_ASSERT(ret == SORM_OK);
    ret = text_blob_set_text_stack(text_blob, "text_stack");
    CU_ASSERT(ret == SORM_OK);

    ret = text_blob_insert(conn, text_blob);
    CU_ASSERT(ret == SORM_OK);

    ret = text_blob_select_by_id(conn, NULL, ALL_COLUMNS, 1, &select_text_blob);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(strcmp(text_blob->text_heap, select_text_blob->text_heap) == 0);
    CU_ASSERT(strcmp(text_blob->text_stack, select_text_blob->text_stack) == 0);
    text_blob_free(NULL, select_text_blob);

    ret = text_blob_delete_by_id(conn, 1);
    CU_ASSERT(ret == SORM_OK);

    text_blob_free(NULL, text_blob);

    /* with NULL */
    text_blob = text_blob_new(NULL);
    CU_ASSERT(text_blob != NULL);

    ret = text_blob_set_id(text_blob, 1);

    ret = text_blob_insert(conn, text_blob);
    CU_ASSERT(ret == SORM_OK);

    ret = text_blob_select_by_id(conn, NULL, ALL_COLUMNS, 1, &select_text_blob);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(text_blob->text_heap == NULL);
    CU_ASSERT(select_text_blob->text_heap == NULL);
    CU_ASSERT(strlen(text_blob->text_stack) == 0);
    CU_ASSERT(strlen(select_text_blob->text_stack) == 0);
    text_blob_free(NULL, select_text_blob);

    ret = text_blob_delete_by_id(conn, 1);
    CU_ASSERT(ret == SORM_OK);

    text_blob_free(NULL, text_blob);
}

static void test_single_quote(void)
{
    device_t *device, *select_device;

    char *uuid = "single'quote";
    int ret;

    device = device_new(NULL);
    device_set_uuid(device, uuid);
    ret = device_insert(conn, device);
    CU_ASSERT(ret == SORM_OK);
    ret = device_select_by_uuid(conn, NULL, "*", uuid, &select_device);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(strcmp(select_device->uuid, uuid) == 0);
    ret = device_delete(conn, device);
    CU_ASSERT(ret == SORM_OK);
    ret = device_select_by_uuid(conn, NULL, "*", uuid, &select_device);
    CU_ASSERT(ret == SORM_NOEXIST);

    ret = device_insert(conn, device);
    ret = device_delete_by_uuid(conn, uuid);
    ret = device_select_by_uuid(conn, NULL, "*", uuid, &select_device);
    CU_ASSERT(ret == SORM_NOEXIST);
}

static void test_double_quote(void)
{
    device_t *device, *select_device;

    char *uuid = "double\"quote";
    int ret;

    device = device_new(NULL);
    device_set_uuid(device, uuid);
    ret = device_insert(conn, device);
    CU_ASSERT(ret == SORM_OK);
    ret = device_select_by_uuid(conn, NULL, "*", uuid, &select_device);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(strcmp(select_device->uuid, uuid) == 0);
    ret = device_delete(conn, device);
    CU_ASSERT(ret == SORM_OK);
    ret = device_select_by_uuid(conn, NULL, "*", uuid, &select_device);
    CU_ASSERT(ret == SORM_NOEXIST);

    ret = device_insert(conn, device);
    ret = device_delete_by_uuid(conn, uuid);
    ret = device_select_by_uuid(conn, NULL, "*", uuid, &select_device);
    CU_ASSERT(ret == SORM_NOEXIST);
}


static void test_int64(void)
{
    device_t *device, *select_device;

    device = device_new(NULL);

#define SMALL_NUM 123456
#define BIG_NUM 123456789012

    device_set_small_num(device, SMALL_NUM);
    device_set_big_num(device, BIG_NUM);

    device_insert(conn, device);
    
    CU_ASSERT(device->small_num == SMALL_NUM);
    CU_ASSERT(device->big_num == BIG_NUM);

    device_select_by_id(conn, NULL, "*", device->id, &select_device);
    CU_ASSERT(device->small_num == select_device->small_num);
    CU_ASSERT(device->big_num == select_device->big_num);

    device_free(NULL, device);
    device_free(NULL, select_device);
    
    device = device_new(NULL);
    
    device_set_small_num(device, BIG_NUM);

    device_insert(conn, device);
    
    CU_ASSERT(device->small_num != BIG_NUM);

    device_select_by_id(conn, NULL, "*", device->id, &select_device);
    CU_ASSERT(device->small_num == select_device->small_num);

    device_free(NULL, device);
    device_free(NULL, select_device);
}

static void test_blob(void)
{
    text_blob_t *text_blob, *select_text_blob;
    int ret;
    blob_t blob, *blob_p;
    blob.i = 38, blob.d = 343.3242;

    text_blob = text_blob_new(NULL);
    CU_ASSERT(text_blob != NULL);

    ret = text_blob_set_id(text_blob, 1);
    ret = text_blob_set_blob_heap(text_blob, &blob, sizeof(blob_t));
    CU_ASSERT(ret == SORM_OK);
    blob_p = text_blob->blob_heap;
    CU_ASSERT(blob.i == blob_p->i);
    CU_ASSERT(blob.d == blob_p->d);
    ret = text_blob_set_blob_stack(text_blob, &blob, sizeof(blob_t));
    CU_ASSERT(ret == SORM_OK);
    blob_p = (blob_t*)text_blob->blob_stack;
    CU_ASSERT(blob.i == blob_p->i);
    CU_ASSERT(blob.d == blob_p->d);

    ret = text_blob_insert(conn, text_blob);
    CU_ASSERT(ret == SORM_OK);

    ret = text_blob_select_by_id(conn, NULL, ALL_COLUMNS, 1, &select_text_blob);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(sizeof(blob_t) == select_text_blob->blob_stack_len);
    blob_p = (blob_t*)select_text_blob->blob_stack;
    CU_ASSERT(blob.i == blob_p->i);
    CU_ASSERT(blob.d == blob_p->d);
    CU_ASSERT(sizeof(blob_t) == select_text_blob->blob_heap_len);
    blob_p = select_text_blob->blob_heap;
    assert(blob_p != NULL);
    CU_ASSERT(blob.i == blob_p->i);
    CU_ASSERT(blob.d == blob_p->d);
    text_blob_free(NULL, select_text_blob);

    ret = text_blob_delete_by_id(conn, 1);
    CU_ASSERT(ret == SORM_OK);

    text_blob_free(NULL, text_blob);

    /* with NULL */
    text_blob = text_blob_new(NULL);
    CU_ASSERT(text_blob != NULL);

    ret = text_blob_set_id(text_blob, 1);

    ret = text_blob_insert(conn, text_blob);
    CU_ASSERT(ret == SORM_OK);

    ret = text_blob_select_by_id(conn, NULL, ALL_COLUMNS, 1, &select_text_blob);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(0 == text_blob->blob_heap_len);
    CU_ASSERT(0 == select_text_blob->blob_heap_len);
    CU_ASSERT(text_blob->blob_heap == NULL);
    CU_ASSERT(select_text_blob->blob_heap == NULL);
    CU_ASSERT(0 == text_blob->blob_heap_len);
    CU_ASSERT(0 == select_text_blob->blob_heap_len);
    CU_ASSERT(strlen(text_blob->blob_stack) == 0);
    CU_ASSERT(strlen(select_text_blob->blob_stack) == 0);
    text_blob_free(NULL, select_text_blob);

    ret = text_blob_delete_by_id(conn, 1);
    CU_ASSERT(ret == SORM_OK);

    text_blob_free(NULL, text_blob);
}

/** @brief: when the sorm object use heap to store text, test select result */
static void test_heap_select(void)
{
    text_blob_t *text_blob, *select_text_blob;
    sorm_list_t *list;
    int amount = 10;
    int i, ret, n;
    /*insert some value for select*/
    text_blob = text_blob_new(NULL);
    for(i = 0; i < amount; i ++)
    {
        ret = text_blob_set_id(text_blob, i);
        CU_ASSERT(ret == SORM_OK);
        ret = text_blob_set_text_heap(text_blob, "text_heap");
        CU_ASSERT(ret == SORM_OK);
        ret = text_blob_insert(conn, text_blob);
        CU_ASSERT(ret == SORM_OK);
    }
    text_blob_free(NULL, text_blob);
    /*****************************/
    /* test select_some_array_by */
    /*****************************/
    //printf("Start select half by select_some_array_by without filter\n");
    n = amount / 2;
    ret = text_blob_select_some_array_by(conn, NULL,  ALL_COLUMNS, NULL, &n, 
            &select_text_blob);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == (amount / 2));
    text_blob_free_array(NULL, select_text_blob, n);

    ret = text_blob_select_all_array_by(conn, NULL,  ALL_COLUMNS, NULL, &n, 
            &select_text_blob);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == amount);
    text_blob_free_array(NULL, select_text_blob, n);

    n = amount / 2;
    ret = text_blob_select_some_list_by(conn, NULL,  ALL_COLUMNS, NULL, &n,
            &list);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == (amount /2));
    sorm_list_free(NULL, list);

    ret = text_blob_select_all_list_by(conn, NULL,  ALL_COLUMNS, NULL, &n,
            &list);
    CU_ASSERT(ret == SORM_OK);
    CU_ASSERT(n == amount);
    sorm_list_free(NULL, list);
}

static void test_tb_to_string(void)
{
    text_blob_t *text_blob;
    blob_t blob;
    char string[1024];
    int ret;

    text_blob = text_blob_new(NULL);
    text_blob_set_blob_heap(text_blob, &blob, sizeof(blob_t));
    text_blob_set_blob_stack(text_blob, &blob, sizeof(blob_t));

    char string1[1024];
    sprintf(string1, "text_blob { id(null); text_heap(null); "
            "text_stack(null); blob_heap(%zd); blob_stack(%zd); }", 
            sizeof(blob_t), sizeof(blob_t));

    text_blob_to_string(text_blob, string, 1024);
    CU_ASSERT(strcmp(string, string1) == 0);

    text_blob_free(NULL, text_blob);
}

static CU_TestInfo tests_device[] = {
    {"01.test_device_new", test_device_new},
    {"02.test_device_ssd", test_device_ssd},
    {"03.test_device_update", test_device_update},
    {"04.test_device_free", test_device_free},
    {"05.test_device_select", test_device_select},
    {"06.test_device_select_too_long", test_device_select_too_long},
    {"07.test_device_noexist", test_device_noexist},
    {"08.test_transaction", test_transaction},
    {"09.test_delete_by_id", test_delete_by_id},
    {"10.test_select_columns", test_select_columns},
    {"11.test_select_by_column", test_select_by_column},
    {"12.test_select_null", test_select_null},
    {"13.test_index", test_index},
    {"14.test_create_drop_conflict", test_create_drop_conflict},
    {"15.test_PK", test_PK}, 
    {"16.test_to_string", test_to_string},
    {"17.test_int64", test_int64},
    {"18.test_delete_by", test_delete_by},
    {"19.test_single_quote", test_single_quote},
    {"19.test_double_quote", test_single_quote},
    CU_TEST_INFO_NULL,
};

static CU_TestInfo tests_sorm[] = {
    {"01.test_sorm_select_by_join", test_sorm_select_by_join},
    {"02.test_sorm_select_columns_by_join", test_sorm_select_columns_by_join},
    {"03.test_sorm_select_null_by_join", test_sorm_select_null_by_join},
    {"05.test_volume_select_by_driver", test_volume_select_by_driver},
    {"05.test_sorm_strerror", test_sorm_strerror},
    {"06.test_sorm_unique", test_sorm_unique},
    {"07.test_sorm_foreign_key", test_sorm_foreign_key},
    {"08.test_pthread_transaction", test_pthread_transaction}, 
    CU_TEST_INFO_NULL,
};

static CU_TestInfo tests_text_blob[] = {
    {"01.test_text", test_text},
    {"02.test_blob", test_blob}, 
    {"03.test_heap_select", test_heap_select},
    {"04.test_to_string", test_tb_to_string},
    CU_TEST_INFO_NULL,
};


static CU_SuiteInfo suites[] = {
    {"TestDevice", suite_sorm_init, suite_sorm_final, tests_device},
    //{"TestSorm", suite_sorm_init, suite_sorm_final, tests_sorm},
    //{"TestTextBlob", suite_sorm_init, suite_sorm_final, tests_text_blob},
    CU_SUITE_INFO_NULL,
};

void AddTests()
{
    assert(NULL != CU_get_registry());
    assert(!CU_is_test_running());

    if (CU_register_suites(suites) != CUE_SUCCESS) {
        fprintf(stderr, "suit registry failed %s\n", CU_get_error_msg());
        exit(EXIT_FAILURE);
    }
}

