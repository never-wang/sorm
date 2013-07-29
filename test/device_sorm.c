#include "device_sorm.h"
static sorm_column_descriptor_t device_columns_descriptor[4] =
{
    {
        "id",
        0,
        SORM_TYPE_INT,
        SORM_CONSTRAINT_PK,
        SORM_MEM_NONE,
        DEVICE_ID_MAX_LEN,
        SORM_OFFSET(device_t, id),
        0,
        "NULL",
        "NULL",
    },
    {
        "uuid",
        1,
        SORM_TYPE_TEXT,
        SORM_CONSTRAINT_UNIQUE,
        SORM_MEM_STACK,
        DEVICE_UUID_MAX_LEN,
        SORM_OFFSET(device_t, uuid),
        0,
        "NULL",
        "NULL",
    },
    {
        "name",
        2,
        SORM_TYPE_TEXT,
        SORM_CONSTRAINT_NONE,
        SORM_MEM_STACK,
        DEVICE_NAME_MAX_LEN,
        SORM_OFFSET(device_t, name),
        0,
        "NULL",
        "NULL",
    },
    {
        "password",
        3,
        SORM_TYPE_TEXT,
        SORM_CONSTRAINT_NONE,
        SORM_MEM_STACK,
        DEVICE_PASSWORD_MAX_LEN,
        SORM_OFFSET(device_t, password),
        0,
        "NULL",
        "NULL",
    },
};

static sorm_table_descriptor_t device_table_descriptor =
{
    "device",
    SORM_SIZE(device_t),
    4,
    0,
    device_columns_descriptor,
};

sorm_table_descriptor_t* device_get_desc()
{
    return &device_table_descriptor;
}

device_t* device_new()
{
    return (device_t *)sorm_new(&device_table_descriptor);
}

void device_free(device_t *device)
{
    sorm_free((sorm_table_descriptor_t *)device);
}

int device_create_table(const sorm_connection_t *conn)
{
    return sorm_create_table(conn, &device_table_descriptor);
}

int device_delete_table(const sorm_connection_t *conn)
{
    return sorm_delete_table(conn, &device_table_descriptor);}

int device_save(sorm_connection_t *conn, device_t *device)
{
    return sorm_save(conn, (sorm_table_descriptor_t*)device);
}

int device_set_id(device_t *device, int id)
{
    return sorm_set_column_value((sorm_table_descriptor_t*)device, 0, &id, 0);
}

int device_set_uuid(device_t *device, const char* uuid)
{
    return sorm_set_column_value((sorm_table_descriptor_t*)device, 1, uuid, 0);
}

int device_set_name(device_t *device, const char* name)
{
    return sorm_set_column_value((sorm_table_descriptor_t*)device, 2, name, 0);
}

int device_set_password(device_t *device, const char* password)
{
    return sorm_set_column_value((sorm_table_descriptor_t*)device, 3, password, 0);
}

int device_delete(const sorm_connection_t *conn, const device_t *device)
{
    return sorm_delete(conn, (sorm_table_descriptor_t *)device);
}

int device_delete_by_id(const sorm_connection_t *conn, int id)
{
    return sorm_delete_by_column(
            conn, &device_table_descriptor, 0, (void *)&id);
}

int device_delete_by_uuid(const sorm_connection_t *conn, char* uuid)
{
    return sorm_delete_by_column(
            conn, &device_table_descriptor, 1, (void *)uuid);
}

int device_select_by_id(
        const sorm_connection_t *conn,
        const char *column_names, int id,
        device_t **device)
{
    return sorm_select_one_by_column(
            conn, &device_table_descriptor, column_names,
            0, (void *)&id, (sorm_table_descriptor_t **)device);
}

int device_select_by_uuid(
        const sorm_connection_t *conn,
        const char *column_names,const char* uuid,
        device_t **device)
{
    return sorm_select_one_by_column(
            conn, &device_table_descriptor, column_names,
            1, (void *)uuid, (sorm_table_descriptor_t **)device);
}

int device_select_some_array_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, device_t **devices_array)
{
    return sorm_select_some_array_by(
            conn, &device_table_descriptor, column_names,
            filter, n, (sorm_table_descriptor_t **)devices_array);
}

int device_select_some_list_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, sorm_list_t **devices_list_head)
{
    return sorm_select_some_list_by(
            conn, &device_table_descriptor, column_names,
            filter, n, devices_list_head);
}

int device_select_all_array_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, device_t **devices_array)
{
    return sorm_select_all_array_by(
            conn, &device_table_descriptor, column_names,
            filter, n, (sorm_table_descriptor_t **)devices_array);
}

int device_select_all_list_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, sorm_list_t **devices_list_head)
{
    return sorm_select_all_list_by(
            conn, &device_table_descriptor, column_names,
            filter, n, devices_list_head);
}

int device_create_index(
        const sorm_connection_t *conn, char *columns_name)
{
    return sorm_create_index(conn, &device_table_descriptor, columns_name);
}

int device_drop_index(
        const sorm_connection_t *conn, char *columns_name)
{
    return sorm_drop_index(conn, columns_name);
}

