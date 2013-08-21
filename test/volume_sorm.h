#ifndef VOLUME_SORM_H
#define VOLUME_SORM_H

#include "sorm.h"

#define VOLUME_ID_MAX_LEN 0
#define VOLUME_DEVICE_ID_MAX_LEN 0
#define VOLUME_UUID_MAX_LEN 31
#define VOLUME_DRIVE_MAX_LEN 15
#define VOLUME_LABEL_MAX_LEN 31

#define VOLUME_DESC volume_get_desc()

#define volume_list_for_each(data, pos, head) \
    sorm_list_data_for_each(data, volume_t, pos, head)
#define volume_list_for_each_safe(data, pos, scratch, head) \
    sorm_list_data_for_each_safe(data, volume_t, pos, scratch, head)

typedef struct volume_s
{
    sorm_table_descriptor_t table_desc;

    sorm_stat_t id_stat;
    int         id;

    sorm_stat_t device_id_stat;
    int         device_id;

    sorm_stat_t uuid_stat;
    char        uuid[VOLUME_UUID_MAX_LEN + 1];

    sorm_stat_t drive_stat;
    char        drive[VOLUME_DRIVE_MAX_LEN + 1];

    sorm_stat_t label_stat;
    char        label[VOLUME_LABEL_MAX_LEN + 1];

} volume_t;

sorm_table_descriptor_t* volume_get_desc();

volume_t* volume_new();

void volume_free(volume_t *volume);

void volume_free_array(volume_t *volume, int n);

int volume_create_table(const sorm_connection_t *conn);

int volume_delete_table(const sorm_connection_t *conn);

int volume_save(sorm_connection_t *conn, volume_t *volume);

int volume_set_id(volume_t *volume, int id);
int volume_set_device_id(volume_t *volume, int device_id);
int volume_set_uuid(volume_t *volume, const char* uuid);
int volume_set_drive(volume_t *volume, const char* drive);
int volume_set_label(volume_t *volume, const char* label);

int volume_delete(const sorm_connection_t *conn, const volume_t *volume);

int volume_delete_by_id(const sorm_connection_t *conn, int id);
int volume_delete_by_uuid(const sorm_connection_t *conn, char* uuid);

int volume_select_by_id(
    const sorm_connection_t *conn,
    const char *column_names, int id,
    volume_t **volume);
int volume_select_by_uuid(
    const sorm_connection_t *conn,
    const char *column_names, const char* uuid,
    volume_t **volume);

int volume_select_some_array_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, volume_t **volumes_array);
int volume_select_some_list_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, sorm_list_t **volumes_list_head);
int volume_select_all_array_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, volume_t **volumes_array);
int volume_select_all_list_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, sorm_list_t **volumes_list_head);

int volume_select_some_array_by_device(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, volume_t **volumes_array);
int volume_select_some_list_by_device(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, sorm_list_t **volumes_list_head);
int volume_select_all_array_by_device(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, volume_t **volumes_array);
int volume_select_all_list_by_device(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, sorm_list_t **volumes_list_head);

int volume_create_index(
        const sorm_connection_t *conn, char *columns_name);
int volume_drop_index(
        const sorm_connection_t *conn, char *columns_name);
#endif