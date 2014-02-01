#ifndef TEST_INDEX_SORM_H
#define TEST_INDEX_SORM_H

#include "sorm.h"

#define TEST_INDEX__ID__MAX_LEN 0
#define TEST_INDEX__UUID__MAX_LEN 31
#define TEST_INDEX__NAME__MAX_LEN 31

#define TEST_INDEX__ALL_COLUMNS "test_index.*"
#define TABLE__TEST_INDEX "test_index"
#define COLUMN__TEST_INDEX__ID "test_index.id"
#define COLUMN__TEST_INDEX__UUID "test_index.uuid"
#define COLUMN__TEST_INDEX__NAME "test_index.name"

#define DESC__TEST_INDEX test_index_get_desc()

#define test_index_list_for_each(data, pos, head) \
    sorm_list_data_for_each(data, test_index_t, pos, head)
#define test_index_list_for_each_safe(data, pos, scratch, head) \
    sorm_list_data_for_each_safe(data, test_index_t, pos, scratch, head)

typedef struct test_index_s
{
    sorm_table_descriptor_t table_desc;

    sorm_stat_t id_stat;
    int         id;

    sorm_stat_t uuid_stat;
    char        uuid[TEST_INDEX__UUID__MAX_LEN + 1];

    sorm_stat_t name_stat;
    char        name[TEST_INDEX__NAME__MAX_LEN + 1];

} test_index_t;

#define TEST_INDEX_INIT { test_index_table_descriptor, \
    0, 0, 0, NULL, 0, NULL,  }

extern sorm_table_descriptor_t test_index_table_descriptor;

static inline test_index_init(test_index_t *test_index) {
    bzero(test_index, sizeof(test_index_t)); 
    test_index->table_desc = test_index_table_descriptor; 
}

char* test_index_to_string(
        test_index_t *test_index, char *string, int len);

sorm_table_descriptor_t* test_index_get_desc();

test_index_t* test_index_new();

void test_index_free(test_index_t *test_index);

void test_index_free_array(test_index_t *test_index, int n);

int test_index_create_table(const sorm_connection_t *conn);

int test_index_delete_table(const sorm_connection_t *conn);

int test_index_insert(sorm_connection_t *conn, test_index_t *test_index);

int test_index_update(sorm_connection_t *conn, test_index_t *test_index);

int test_index_set_id(test_index_t *test_index, int id);
int test_index_set_uuid(test_index_t *test_index, const char* uuid);
int test_index_set_name(test_index_t *test_index, const char* name);

int test_index_delete(const sorm_connection_t *conn, const test_index_t *test_index);

int test_index_delete_by(const sorm_connection_t *conn, const char *filter);

int test_index_delete_by_id(const sorm_connection_t *conn, int id);

int test_index_select_by_id(
        const sorm_connection_t *conn,
        const char *column_names, int id,
        test_index_t **test_index);

int test_index_select_some_array_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, test_index_t **test_indexs_array);
int test_index_select_some_list_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, sorm_list_t **test_indexs_list_head);
int test_index_select_all_array_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, test_index_t **test_indexs_array);
int test_index_select_all_list_by(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        int *n, sorm_list_t **test_indexs_list_head);

int test_index_select_iterate_by_open(
        const sorm_connection_t *conn,
        const char *column_names, const char *filter,
        sorm_iterator_t **iterator);
int test_index_select_iterate_by(
        sorm_iterator_t *iterator, test_index_t **test_index);
#define test_index_select_iterate_close sorm_select_iterate_close
#define test_index_select_iterate_more sorm_select_iterate_more
int test_index_create_index(
        const sorm_connection_t *conn, char *columns_name);
int test_index_drop_index(
        const sorm_connection_t *conn, char *columns_name);
#endif