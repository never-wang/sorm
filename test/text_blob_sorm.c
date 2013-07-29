#include "text_blob_sorm.h"
static sorm_column_descriptor_t text_blob_columns_descriptor[5] =
{
    {
        "id",
        0,
        SORM_TYPE_INT,
        SORM_CONSTRAINT_PK,
        SORM_MEM_NONE,
        TEXT_BLOB_ID_MAX_LEN,
        SORM_OFFSET(text_blob_t, id),
        0,
        "NULL",
        "NULL",
    },
    {
        "text_heap",
        1,
        SORM_TYPE_TEXT,
        SORM_CONSTRAINT_NONE,
        SORM_MEM_HEAP,
        TEXT_BLOB_TEXT_HEAP_MAX_LEN,
        SORM_OFFSET(text_blob_t, text_heap),
        0,
        "NULL",
        "NULL",
    },
    {
        "text_stack",
        2,
        SORM_TYPE_TEXT,
        SORM_CONSTRAINT_NONE,
        SORM_MEM_STACK,
        TEXT_BLOB_TEXT_STACK_MAX_LEN,
        SORM_OFFSET(text_blob_t, text_stack),
        0,
        "NULL",
        "NULL",
    },
    {
        "blob_heap",
        3,
        SORM_TYPE_BLOB,
        SORM_CONSTRAINT_NONE,
        SORM_MEM_HEAP,
        TEXT_BLOB_BLOB_HEAP_MAX_LEN,
        SORM_OFFSET(text_blob_t, blob_heap),
        0,
        "NULL",
        "NULL",
    },
    {
        "blob_stack",
        4,
        SORM_TYPE_BLOB,
        SORM_CONSTRAINT_NONE,
        SORM_MEM_STACK,
        TEXT_BLOB_BLOB_STACK_MAX_LEN,
        SORM_OFFSET(text_blob_t, blob_stack),
        0,
        "NULL",
        "NULL",
    },
};

static sorm_table_descriptor_t text_blob_table_descriptor =
{
    "text_blob",
    SORM_SIZE(text_blob_t),
    5,
    0,
    text_blob_columns_descriptor,
};

sorm_table_descriptor_t* text_blob_get_desc()
{
    return &text_blob_table_descriptor;
}

text_blob_t* text_blob_new()
{
    return (text_blob_t *)sorm_new(&text_blob_table_descriptor);
}

void text_blob_free(text_blob_t *text_blob)
{
    sorm_free((sorm_table_descriptor_t *)text_blob);
}

int text_blob_create_table(const sorm_connection_t *conn)
{
    return sorm_create_table(conn, &text_blob_table_descriptor);
}

int text_blob_delete_table(const sorm_connection_t *conn)
{
    return sorm_delete_table(conn, &text_blob_table_descriptor);}

int text_blob_save(sorm_connection_t *conn, text_blob_t *text_blob)
{
    return sorm_save(conn, (sorm_table_descriptor_t*)text_blob);
}

int text_blob_set_id(text_blob_t *text_blob, int id)
{
    return sorm_set_column_value((sorm_table_descriptor_t*)text_blob, 0, &id, 0);
}

int text_blob_set_text_heap(text_blob_t *text_blob, const char* text_heap)
{
    return sorm_set_column_value((sorm_table_descriptor_t*)text_blob, 1, text_heap, 0);
}

int text_blob_set_text_stack(text_blob_t *text_blob, const char* text_stack)
{
    return sorm_set_column_value((sorm_table_descriptor_t*)text_blob, 2, text_stack, 0);
}

int text_blob_set_blob_heap(text_blob_t *text_blob, const void* blob_heap, int len)
{
    return sorm_set_column_value((sorm_table_descriptor_t*)text_blob, 3, blob_heap, len);
}

int text_blob_set_blob_stack(text_blob_t *text_blob, const void* blob_stack, int len)
{
    return sorm_set_column_value((sorm_table_descriptor_t*)text_blob, 4, blob_stack, len);
}

int text_blob_delete(const sorm_connection_t *conn, const text_blob_t *text_blob)
{
    return sorm_delete(conn, (sorm_table_descriptor_t *)text_blob);
}

int text_blob_delete_by_id(const sorm_connection_t *conn, int id)
{
    return sorm_delete_by_column(
            conn, &text_blob_table_descriptor, 0, (void *)&id);
}

int text_blob_select_by_id(
        const sorm_connection_t *conn,
        const char *column_names, int id,
        text_blob_t **text_blob)
{
    return sorm_select_one_by_column(
            conn, &text_blob_table_descriptor, column_names,
            0, (void *)&id, (sorm_table_descriptor_t **)text_blob);
}

int text_blob_select_some_array_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, text_blob_t **text_blobs_array)
{
    return sorm_select_some_array_by(
            conn, &text_blob_table_descriptor, column_names,
            filter, n, (sorm_table_descriptor_t **)text_blobs_array);
}

int text_blob_select_some_list_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, sorm_list_t **text_blobs_list_head)
{
    return sorm_select_some_list_by(
            conn, &text_blob_table_descriptor, column_names,
            filter, n, text_blobs_list_head);
}

int text_blob_select_all_array_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, text_blob_t **text_blobs_array)
{
    return sorm_select_all_array_by(
            conn, &text_blob_table_descriptor, column_names,
            filter, n, (sorm_table_descriptor_t **)text_blobs_array);
}

int text_blob_select_all_list_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, sorm_list_t **text_blobs_list_head)
{
    return sorm_select_all_list_by(
            conn, &text_blob_table_descriptor, column_names,
            filter, n, text_blobs_list_head);
}

int text_blob_create_index(
        const sorm_connection_t *conn, char *columns_name)
{
    return sorm_create_index(conn, &text_blob_table_descriptor, columns_name);
}

int text_blob_drop_index(
        const sorm_connection_t *conn, char *columns_name)
{
    return sorm_drop_index(conn, columns_name);
}

