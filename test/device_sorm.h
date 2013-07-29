#ifndef DEVICE_SORM_H
#define DEVICE_SORM_H

#include "sorm.h"

#define DEVICE_ID_MAX_LEN 0
#define DEVICE_UUID_MAX_LEN 31
#define DEVICE_NAME_MAX_LEN 63
#define DEVICE_PASSWORD_MAX_LEN 63

#define DEVICE_DESC device_get_desc()

typedef struct device_s
{
    sorm_table_descriptor_t table_desc;

    sorm_stat_t id_stat;
    int         id;

    sorm_stat_t uuid_stat;
    char        uuid[DEVICE_UUID_MAX_LEN + 1];

    sorm_stat_t name_stat;
    char        name[DEVICE_NAME_MAX_LEN + 1];

    sorm_stat_t password_stat;
    char        password[DEVICE_PASSWORD_MAX_LEN + 1];

} device_t;

sorm_table_descriptor_t* device_get_desc();

device_t* device_new();

void device_free(device_t *device);

int device_create_table(const sorm_connection_t *conn);

int device_delete_table(const sorm_connection_t *conn);

int device_save(sorm_connection_t *conn, device_t *device);

int device_set_id(device_t *device, int id);
int device_set_uuid(device_t *device, const char* uuid);
int device_set_name(device_t *device, const char* name);
int device_set_password(device_t *device, const char* password);

int device_delete(const sorm_connection_t *conn, const device_t *device);

int device_delete_by_id(const sorm_connection_t *conn, int id);
int device_delete_by_uuid(const sorm_connection_t *conn, char* uuid);

int device_select_by_id(
    const sorm_connection_t *conn,
    const char *column_names, int id,
    device_t **device);
int device_select_by_uuid(
    const sorm_connection_t *conn,
    const char *column_names, const char* uuid,
    device_t **device);

int device_select_some_array_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, device_t **devices_array);
int device_select_some_list_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, sorm_list_t **devices_list_head);
int device_select_all_array_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, device_t **devices_array);
int device_select_all_list_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, sorm_list_t **devices_list_head);

int device_create_index(
        const sorm_connection_t *conn, char *columns_name);
int device_drop_index(
        const sorm_connection_t *conn, char *columns_name);
#endif