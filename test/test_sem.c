/****************************************************************************
 *       Filename:  test_db.c
 *
 *    Description:  :
 *
 *        Version:  1.0
 *        Created:  03/25/13 13:20:24
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *       Email:  never.wencan@gmail.com
 ***************************************************************************/
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "sorm.h"
#include "device_sorm.h"

#define DB_FILE "test.db"
#define SEM_KEY 1025

int main()
{
    sorm_connection_t *conn;
    int ret;
    int i;
    char buf[128];
    device_t *device;

    ret = sorm_init(0);
    if(ret != SORM_OK)
    {
        return -1;
    }

    if(ret != 0)
    {
        return -1;
    }

    ret = sorm_open(DB_FILE, SORM_DB_SQLITE, SEM_KEY,
            SORM_ENABLE_SEMAPHORE, &conn);

    if(ret != SORM_OK)
    {
        return -1;
    }

    device_create_table(conn);

    device = device_new();
    device_set_name(device, "test_device");
    device_set_password(device, "654321");

    i = 0;
    while(1)
    {
        device_set_id(device, i);
        sprintf(buf, "%d", i);
        device_set_uuid(device, buf);
        device_save(conn, device);

        i ++;
    }

    device_free(device);
    sorm_close(conn);
    sorm_final();
}
