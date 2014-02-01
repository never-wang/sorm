#include "test_index_sorm.h"
static sorm_column_descriptor_t test_index_columns_descriptor[3] =
{
    {
        "id",
        0,
        SORM_TYPE_INT,
        SORM_CONSTRAINT_PK,
        SORM_MEM_NONE,
        TEST_INDEX__ID__MAX_LEN,
        SORM_OFFSET(test_index_t, id),
        0,
        "NULL",
        "NULL",
    },
    {
        "uuid",
        1,
        SORM_TYPE_TEXT,
        SORM_CONSTRAINT_NONE,
        SORM_MEM_STACK,
        TEST_INDEX__UUID__MAX_LEN,
        SORM_OFFSET(test_index_t, uuid),
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
        TEST_INDEX__NAME__MAX_LEN,
        SORM_OFFSET(test_index_t, name),
        0,
        "NULL",
        "NULL",
    },
};

sorm_table_descriptor_t test_index_table_descriptor =
{
    "test_index",
    SORM_SIZE(test_index_t),
    3,
    0,
    "CREATE TABLE IF NOT EXISTS test_index(id INTEGER PRIMARY KEY, uuid TEXT(31), name TEXT(31), UNIQUE(uuid, name))",
    test_index_columns_descriptor,
};

char* test_index_to_string(
        test_index_t *test_index, char *string, int len)
{
    return sorm_to_string((sorm_table_descriptor_t*)test_index, string, len);
}

sorm_table_descriptor_t* test_index_get_desc()
{
    return &test_index_table_descriptor;
}

test_index_t* test_index_new()
{
    return (test_index_t *)sorm_new(&test_index_table_descriptor);
}

void test_index_free(test_index_t *test_index)
{
    sorm_free((sorm_table_descriptor_t *)test_index);
}

void test_index_free_array(test_index_t *test_index, int n)
{
    sorm_free_array((sorm_table_descriptor_t *)test_index, n);
}

int test_index_create_table(const sorm_connection_t *conn)
{
    char *sql_stmt = "CREATE TABLE IF NOT EXISTS test_index(id INTEGER PRIMARY KEY, uuid TEXT(31), name TEXT(31), UNIQUE(uuid, name))";

    return sorm_run_stmt(conn, sql_stmt, SORM_RWLOCK_WRITE);
}

int test_index_delete_table(const sorm_connection_t *conn)
{
    return sorm_delete_table(conn, &test_index_table_descriptor);}

int test_index_insert(sorm_connection_t *conn, test_index_t *test_index)
{
    return sorm_insert(conn, (sorm_table_descriptor_t*)test_index);
}

int test_index_update(sorm_connection_t *conn, test_index_t *test_index)
{
    return sorm_update(conn, (sorm_table_descriptor_t*)test_index);
}

int test_index_set_id(test_index_t *test_index, int id)
{
    return sorm_set_column_value((sorm_table_descriptor_t*)test_index, 0, &id, 0);
}

int test_index_set_uuid(test_index_t *test_index, const char* uuid)
{
    return sorm_set_column_value((sorm_table_descriptor_t*)test_index, 1, uuid, 0);
}

int test_index_set_name(test_index_t *test_index, const char* name)
{
    return sorm_set_column_value((sorm_table_descriptor_t*)test_index, 2, name, 0);
}

int test_index_delete(const sorm_connection_t *conn, const test_index_t *test_index)
{
    return sorm_delete(conn, (sorm_table_descriptor_t *)test_index);
}

int test_index_delete_by(const sorm_connection_t *conn, const char *filter)
{
    return sorm_delete_by(conn, &test_index_table_descriptor, filter);
}

int test_index_delete_by_id(const sorm_connection_t *conn, int id)
{
    return sorm_delete_by_column(
            conn, &test_index_table_descriptor, 0, (void *)&id);
}

int test_index_select_by_id(
        const sorm_connection_t *conn,
        const char *column_names, int id,
        test_index_t **test_index)
{
    return sorm_select_one_by_column(
            conn, &test_index_table_descriptor, column_names,
            0, (void *)&id, (sorm_table_descriptor_t **)test_index);
}

int test_index_select_some_array_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, test_index_t **test_indexs_array)
{
    return sorm_select_some_array_by(
            conn, &test_index_table_descriptor, column_names,
            filter, n, (sorm_table_descriptor_t **)test_indexs_array);
}

int test_index_select_some_list_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, sorm_list_t **test_indexs_list_head)
{
    return sorm_select_some_list_by(
            conn, &test_index_table_descriptor, column_names,
            filter, n, test_indexs_list_head);
}

int test_index_select_all_array_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, test_index_t **test_indexs_array)
{
    return sorm_select_all_array_by(
            conn, &test_index_table_descriptor, column_names,
            filter, n, (sorm_table_descriptor_t **)test_indexs_array);
}

int test_index_select_all_list_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, sorm_list_t **test_indexs_list_head)
{
    return sorm_select_all_list_by(
            conn, &test_index_table_descriptor, column_names,
            filter, n, test_indexs_list_head);
}

int test_index_select_iterate_by_open(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        sorm_iterator_t **iterator)
{
    return sorm_select_iterate_by_open(
            conn, &test_index_table_descriptor, column_names,
            filter, iterator);
}

int test_index_select_iterate_by(
        sorm_iterator_t *iterator, test_index_t **test_index)
{
    return sorm_select_iterate_by(
            iterator, (sorm_table_descriptor_t **)test_index);
}

int test_index_create_index(
        const sorm_connection_t *conn, char *columns_name)
{
    return sorm_create_index(conn, &test_index_table_descriptor, columns_name);
}

int test_index_drop_index(
        const sorm_connection_t *conn, char *columns_name)
{
    return sorm_drop_index(conn, columns_name);
}

