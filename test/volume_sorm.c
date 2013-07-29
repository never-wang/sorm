#include "volume_sorm.h"
#include "device_sorm.h"
static sorm_column_descriptor_t volume_columns_descriptor[5] =
{
    {
        "id",
        0,
        SORM_TYPE_INT,
        SORM_CONSTRAINT_PK,
        SORM_MEM_NONE,
        VOLUME_ID_MAX_LEN,
        SORM_OFFSET(volume_t, id),
        0,
        "NULL",
        "NULL",
    },
    {
        "device_id",
        1,
        SORM_TYPE_INT,
        SORM_CONSTRAINT_NONE,
        SORM_MEM_NONE,
        VOLUME_DEVICE_ID_MAX_LEN,
        SORM_OFFSET(volume_t, device_id),
        1,
        "device",
        "id",
    },
    {
        "uuid",
        2,
        SORM_TYPE_TEXT,
        SORM_CONSTRAINT_UNIQUE,
        SORM_MEM_STACK,
        VOLUME_UUID_MAX_LEN,
        SORM_OFFSET(volume_t, uuid),
        0,
        "NULL",
        "NULL",
    },
    {
        "drive",
        3,
        SORM_TYPE_TEXT,
        SORM_CONSTRAINT_NONE,
        SORM_MEM_STACK,
        VOLUME_DRIVE_MAX_LEN,
        SORM_OFFSET(volume_t, drive),
        0,
        "NULL",
        "NULL",
    },
    {
        "label",
        4,
        SORM_TYPE_TEXT,
        SORM_CONSTRAINT_NONE,
        SORM_MEM_STACK,
        VOLUME_LABEL_MAX_LEN,
        SORM_OFFSET(volume_t, label),
        0,
        "NULL",
        "NULL",
    },
};

static sorm_table_descriptor_t volume_table_descriptor =
{
    "volume",
    SORM_SIZE(volume_t),
    5,
    0,
    volume_columns_descriptor,
};

sorm_table_descriptor_t* volume_get_desc()
{
    return &volume_table_descriptor;
}

volume_t* volume_new()
{
    return (volume_t *)sorm_new(&volume_table_descriptor);
}

void volume_free(volume_t *volume)
{
    sorm_free((sorm_table_descriptor_t *)volume);
}

int volume_create_table(const sorm_connection_t *conn)
{
    return sorm_create_table(conn, &volume_table_descriptor);
}

int volume_delete_table(const sorm_connection_t *conn)
{
    return sorm_delete_table(conn, &volume_table_descriptor);}

int volume_save(sorm_connection_t *conn, volume_t *volume)
{
    return sorm_save(conn, (sorm_table_descriptor_t*)volume);
}

int volume_set_id(volume_t *volume, int id)
{
    return sorm_set_column_value((sorm_table_descriptor_t*)volume, 0, &id, 0);
}

int volume_set_device_id(volume_t *volume, int device_id)
{
    return sorm_set_column_value((sorm_table_descriptor_t*)volume, 1, &device_id, 0);
}

int volume_set_uuid(volume_t *volume, const char* uuid)
{
    return sorm_set_column_value((sorm_table_descriptor_t*)volume, 2, uuid, 0);
}

int volume_set_drive(volume_t *volume, const char* drive)
{
    return sorm_set_column_value((sorm_table_descriptor_t*)volume, 3, drive, 0);
}

int volume_set_label(volume_t *volume, const char* label)
{
    return sorm_set_column_value((sorm_table_descriptor_t*)volume, 4, label, 0);
}

int volume_delete(const sorm_connection_t *conn, const volume_t *volume)
{
    return sorm_delete(conn, (sorm_table_descriptor_t *)volume);
}

int volume_delete_by_id(const sorm_connection_t *conn, int id)
{
    return sorm_delete_by_column(
            conn, &volume_table_descriptor, 0, (void *)&id);
}

int volume_delete_by_uuid(const sorm_connection_t *conn, char* uuid)
{
    return sorm_delete_by_column(
            conn, &volume_table_descriptor, 2, (void *)uuid);
}

int volume_select_by_id(
        const sorm_connection_t *conn,
        const char *column_names, int id,
        volume_t **volume)
{
    return sorm_select_one_by_column(
            conn, &volume_table_descriptor, column_names,
            0, (void *)&id, (sorm_table_descriptor_t **)volume);
}

int volume_select_by_uuid(
        const sorm_connection_t *conn,
        const char *column_names,const char* uuid,
        volume_t **volume)
{
    return sorm_select_one_by_column(
            conn, &volume_table_descriptor, column_names,
            2, (void *)uuid, (sorm_table_descriptor_t **)volume);
}

int volume_select_some_array_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, volume_t **volumes_array)
{
    return sorm_select_some_array_by(
            conn, &volume_table_descriptor, column_names,
            filter, n, (sorm_table_descriptor_t **)volumes_array);
}

int volume_select_some_list_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, sorm_list_t **volumes_list_head)
{
    return sorm_select_some_list_by(
            conn, &volume_table_descriptor, column_names,
            filter, n, volumes_list_head);
}

int volume_select_all_array_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, volume_t **volumes_array)
{
    return sorm_select_all_array_by(
            conn, &volume_table_descriptor, column_names,
            filter, n, (sorm_table_descriptor_t **)volumes_array);
}

int volume_select_all_list_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, sorm_list_t **volumes_list_head)
{
    return sorm_select_all_list_by(
            conn, &volume_table_descriptor, column_names,
            filter, n, volumes_list_head);
}

int volume_select_some_array_by_device(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, volume_t **volumes_array)
{
    return sorm_select_some_array_by_join(conn, column_names,
            &volume_table_descriptor, "device_id", DEVICE_DESC, "id", 
            SORM_INNER_JOIN, filter, n, 
            (sorm_table_descriptor_t **)volumes_array, NULL);
}

int volume_select_some_list_by_device(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, sorm_list_t **volumes_list_head)
{
    return sorm_select_some_list_by_join(conn, column_names,
            &volume_table_descriptor, "device_id", DEVICE_DESC, "id", 
            SORM_INNER_JOIN, filter, n, volumes_list_head, NULL);
}

int volume_select_all_array_by_device(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, volume_t **volumes_array)
{
    return sorm_select_all_array_by_join(conn, column_names,
            &volume_table_descriptor, "device_id", DEVICE_DESC, "id", 
            SORM_INNER_JOIN, filter, n, 
            (sorm_table_descriptor_t **)volumes_array, NULL);
}

int volume_select_all_list_by_device(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, sorm_list_t **volumes_list_head)
{
    return sorm_select_all_list_by_join(conn, column_names,
            &volume_table_descriptor, "device_id", DEVICE_DESC, "id", 
            SORM_INNER_JOIN, filter, n, volumes_list_head, NULL);
}

int volume_create_index(
        const sorm_connection_t *conn, char *columns_name)
{
    return sorm_create_index(conn, &volume_table_descriptor, columns_name);
}

int volume_drop_index(
        const sorm_connection_t *conn, char *columns_name)
{
    return sorm_drop_index(conn, columns_name);
}

