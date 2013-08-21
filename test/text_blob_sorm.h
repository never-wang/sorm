#ifndef TEXT_BLOB_SORM_H
#define TEXT_BLOB_SORM_H

#include "sorm.h"

#define TEXT_BLOB_ID_MAX_LEN 0
#define TEXT_BLOB_TEXT_HEAP_MAX_LEN 0
#define TEXT_BLOB_TEXT_STACK_MAX_LEN 31
#define TEXT_BLOB_BLOB_HEAP_MAX_LEN 0
#define TEXT_BLOB_BLOB_STACK_MAX_LEN 31

#define TEXT_BLOB_DESC text_blob_get_desc()

#define text_blob_list_for_each(data, pos, head) \
    sorm_list_data_for_each(data, text_blob_t, pos, head)
#define text_blob_list_for_each_safe(data, pos, scratch, head) \
    sorm_list_data_for_each_safe(data, text_blob_t, pos, scratch, head)

typedef struct text_blob_s
{
    sorm_table_descriptor_t table_desc;

    sorm_stat_t id_stat;
    int         id;

    sorm_stat_t text_heap_stat;
    char*       text_heap;

    sorm_stat_t text_stack_stat;
    char        text_stack[TEXT_BLOB_TEXT_STACK_MAX_LEN + 1];

    int         blob_heap_len;
    sorm_stat_t blob_heap_stat;
    void*       blob_heap;

    int         blob_stack_len;
    sorm_stat_t blob_stack_stat;
    char        blob_stack[TEXT_BLOB_BLOB_STACK_MAX_LEN + 1];

} text_blob_t;

sorm_table_descriptor_t* text_blob_get_desc();

text_blob_t* text_blob_new();

void text_blob_free(text_blob_t *text_blob);

void text_blob_free_array(text_blob_t *text_blob, int n);

int text_blob_create_table(const sorm_connection_t *conn);

int text_blob_delete_table(const sorm_connection_t *conn);

int text_blob_save(sorm_connection_t *conn, text_blob_t *text_blob);

int text_blob_set_id(text_blob_t *text_blob, int id);
int text_blob_set_text_heap(text_blob_t *text_blob, const char* text_heap);
int text_blob_set_text_stack(text_blob_t *text_blob, const char* text_stack);
int text_blob_set_blob_heap(text_blob_t *text_blob, const void* blob_heap, int len);
int text_blob_set_blob_stack(text_blob_t *text_blob, const void* blob_stack, int len);

int text_blob_delete(const sorm_connection_t *conn, const text_blob_t *text_blob);

int text_blob_delete_by_id(const sorm_connection_t *conn, int id);

int text_blob_select_by_id(
    const sorm_connection_t *conn,
    const char *column_names, int id,
    text_blob_t **text_blob);

int text_blob_select_some_array_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, text_blob_t **text_blobs_array);
int text_blob_select_some_list_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, sorm_list_t **text_blobs_list_head);
int text_blob_select_all_array_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, text_blob_t **text_blobs_array);
int text_blob_select_all_list_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, sorm_list_t **text_blobs_list_head);

int text_blob_create_index(
        const sorm_connection_t *conn, char *columns_name);
int text_blob_drop_index(
        const sorm_connection_t *conn, char *columns_name);
#endif