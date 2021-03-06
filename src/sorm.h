/****************************************************************************
 *       Filename:  sorm.h
 *
 *    Description:  define for sorm types
 *
 *        Version:  1.0
 *        Created:  03/11/13 10:43:16
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *        Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#ifndef SORM_H
#define SORM_H

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#include "sqlite3.h"
#include "strings.h"

/* open flags */
#define SORM_ENABLE_FOREIGN_KEY      0x1 /* enable foreign key support in sqlite3 */
#define SORM_ENABLE_SEMAPHORE        0x2    /* enable semaphore in sorm */
/* enable rwlock in sorm, similar with rwlock, two kind of  transaction is provided in the mode,
 * read_transaction alloc a rlock and begin a DEFERRED transaction,
 * write_transaction alloc a wlock and begin a IMMEDIATE transaction.
 * So each operation without a explicit transaction, the read operation will alloc a rlock,
 * the write operation will alloc a wlock*/
#define SORM_ENABLE_RWLOCK           0x4    

#define sorm_foreign_key_enabled(flags) \
    ((SORM_ENABLE_FOREIGN_KEY & (flags)) == SORM_ENABLE_FOREIGN_KEY)
#define sorm_semaphore_enabled(flags) \
    ((SORM_ENABLE_SEMAPHORE & (flags)) == SORM_ENABLE_SEMAPHORE)
#define sorm_rwlock_enabled(flags) \
    ((SORM_ENABLE_RWLOCK & (flags)) == SORM_ENABLE_RWLOCK)

#define SORM_RWLOCK_READ    1
#define SORM_RWLOCK_WRITE   2

#define SORM_OFFSET(struct, member) offsetof(struct, member)
#define SORM_SIZE(struct)    sizeof(struct)

#define ALL_COLUMNS  "*" /* used in select functions, as select * */
#define COLUMN_NAMES_DELIMITER ','
#define TABLE_COLUMN_DELIMITER '.'

#define SQL_STMT_MAX_LEN 1023

#define SORM_NO_PK  -1

#define ALL_INDEXES -1 /* used in parse select columns_name to indexes, means
                          all tables or all columns */
#define NO_INDEXES_IN_RESULT -1 /* used in indexes_in_result , means
                                   the column has not been selected */

#define BUSY_RETRY_TIME 10 /*  if a database  meet busy, 
                              the retry time*/
/* sorm_list */
typedef struct sorm_list_s
{
    struct sorm_list_s *next, *prev;
    void *data;

} sorm_list_t;

#define INIT_LIST_HEAD(head) do { \
    (head)->next = (head); (head)->prev = (head); \
} while (0)

#define sorm_list(pos) \
    ((sorm_list_t*)(((char*)(pos)) - offsetof(sorm_list_t, data)))

#define sorm_list_for_each(pos, head) \
    for(pos = (head)->next; pos != (head); pos = pos->next)

#define sorm_list_for_each_safe(pos, scratch, head) \
    for(pos = (head)->next, scratch = pos->next; pos != (head); \
            pos = scratch, scratch = pos->next)

#define sorm_list_free(allocator, head)\
    _list_free(allocator, head, (void*)(sorm_free))

#define sorm_list_data_for_each(object, type, pos, head) \
    for(pos = (head)->next, object = (type*)(pos->data); pos != (head); \
            pos = pos->next, object = (type*)(pos->data))

#define sorm_list_data_for_each_safe(object, type, pos, head) \
    for(pos = (head)->next, object = (type*)(pos->data), \
            scratch = pos->next; pos != (head); \
            pos = scratch, object = (type*)(pos->data, scratch = pos->next)

static inline void _list_add(
        sorm_list_t *new, sorm_list_t *prev, sorm_list_t *next)
{
    new->next = prev->next;
    prev->next = new;
    new->prev = next->prev;
    next->prev = new;
}

static inline void list_add_tail(sorm_list_t *new, sorm_list_t *head)
{
    _list_add(new, head->prev, head);
}

static inline void list_add_head(sorm_list_t *new, sorm_list_t *head)
{
    _list_add(new, head, head->next);
}


/** @brief: error code for sorm */
typedef enum
{
    SORM_OK            =    0,
    SORM_NOMEM,        
    /* an malloc() fail */
    SORM_DB_ERROR,
    /* error from sqlite3 functions, it may happen if you set the invalid
     * column_names and filter*/
    SORM_TOO_LONG,   
    /* construct too long sql statement or get too long string*/
    SORM_NOPK,
    /* ther is not primary key in the table*/
    SORM_NOEXIST,   
    /* select row does not exist; delete will not return this error */
    SORM_INVALID_NUM, 
    /* Invalid number of select rows */
    SORM_INVALID_COLUMN_NAME,
    /* Invalid column name used in select */
    SORM_INVALID_MEM,
    /* Invalid sorm_mem*/
    SORM_INVALID_TYPE,
    /* Invalid sorm_type_t*/
    SORM_ARG_NULL,
    /* Input argument is NULL */
    SORM_COLUMNS_NAME_EMPTY,
    /* Param columns_name when calling select function is an empty string */
    SORM_FILTER_EMPTY,
    /* Param filter when calling select function is an empty string */
    SORM_INIT_FAIL, /* sorm intialize fail */
    SORM_STRING_BUF_NOT_ENOUGH, 
    /* string buffer is not big enough when transfer a sorm object to string*/
} sorm_error_t;

/** @brief: join type */
typedef enum
{
    SORM_INNER_JOIN    =   0,
    SORM_LEFT_JOIN,
    //SORM_RIGHT_JOIN, /* not supported in sqlite3 */
    //SORM_FULL_JOIN,
} sorm_join_t;

/** @brief: sorm_type_t for value */
typedef enum
{
    SORM_TYPE_INT    =   0,
    SORM_TYPE_INT64,
    SORM_TYPE_TEXT,
    SORM_TYPE_DOUBLE,
    SORM_TYPE_BLOB,    /* BLOB TYPE, for binary data */
} sorm_type_t;

/** @brief: memory type for string */
typedef enum
{
    SORM_MEM_NONE   =   0,
    SORM_MEM_HEAP,
    SORM_MEM_STACK,
} sorm_mem_t;

typedef enum
{
    SORM_CONSTRAINT_NONE    =   0,
    SORM_CONSTRAINT_PK,
    SORM_CONSTRAINT_PK_ASC,
    SORM_CONSTRAINT_PK_DESC,
    SORM_CONSTRAINT_UNIQUE,
} sorm_constraint_t;

#define sorm_is_unique_column(constraint) \
    ((constraint == SORM_CONSTRAINT_PK) || \
     (constraint == SORM_CONSTRAINT_PK_ASC) || \
     (constraint == SORM_CONSTRAINT_PK_DESC) || \
     (constraint == SORM_CONSTRAINT_UNIQUE)) 

/** @brief:  status for the member in an SORM　object*/
#define SORM_STAT_NULL      0x0 /* the member has no value */
#define SORM_STAT_VALUED    0x1 /* the member has a value */
#define SORM_STAT_NEEDFREE  0x2 /* the member need to be freed */
#define sorm_is_stat(stat, check_stat) \
    (((stat) & (check_stat)) == (check_stat))
#define sorm_is_stat_valued(stat) sorm_is_stat(stat, SORM_STAT_VALUED)
#define sorm_is_stat_needfree(stat) sorm_is_stat(stat, SORM_STAT_NEEDFREE)
typedef int sorm_stat_t;

/** @brief: database type */
typedef enum
{
    SORM_DB_SQLITE      =   0,
} sorm_db_t;

/** @brief: descriptor for a column in a table */
typedef struct sorm_column_descriptor_s
{
    char *name;        /* name of the column */
    uint32_t index;    /* index of the column, start form 0 */
    sorm_type_t type;  /* data type of the column */
    sorm_constraint_t constraint;        /* such as PRIMARY KEY, UNIQUE */
    sorm_mem_t mem;      /* useful only when cloumn type is SROM_TYPE_TEXT 
                            and SORM_TYPE_BLOB*/
    int max_len; /* useful only when cloumn type is SORM_TYPE_TEXT and 
                    SORM_TYPE_BLOB*/
    size_t offset;  /* offset of the column in table structure */
    int is_foreign_key;    /* indicates whether this is a foreign key */
    char *foreign_table_name; /* name of foreign table */
    char *foreign_column_name; /* name of foreign column name */
} sorm_column_descriptor_t;

/** @brief: descriptor for a table */
typedef struct sorm_table_descriptor_s
{
    char *name;        /* name of table */
    size_t size;    /* size of talbe row object */

    int columns_num;    /* number of column in the table */
    int PK_index;    /* index for Primary key, SORM_NO_PK means that there is 
                        not Primay key */
    char *create_sql;   /* the sql used to create the table */
    sorm_column_descriptor_t *columns; /* descriptor of columns */
} sorm_table_descriptor_t;

/** @brief: conection to an opened database */
typedef struct sorm_connection_s
{
    sqlite3 *sqlite3_handle;    /* sqlite3 database handle*/

    sorm_db_t db;    /* database type */
    int transaction_num;    /* deal with transaction nest */

    /* semaphore */
    int sem_key;
    pthread_rwlock_t *rwlock;

    int flags;

    /* to store pre-prepared transaction statement */
    //sqlite3_stmt *begin_read_trans_stmt;
    //sqlite3_stmt *begin_write_trans_stmt;
    //sqlite3_stmt *commit_trans_stmt;
    /* remove it , because roolback should be fianlized immediately */
    //sqlite3_stmt *rollback_trans_stmt;
} sorm_connection_t;

/** @brief: used to store the information for selected columns in a table */
typedef struct
{
    int columns_num;    /* the number of selected columns */
    int *indexes_in_result;  /* the index of selected columns in result */
}select_columns_t;

/** @brief: allocator  */
typedef struct sorm_allocator_s
{
    void*(*_alloc)(void *memory_pool, size_t size);
    void(*_free)(void *memory_pool, void *point);
    char*(*_strdup)(void *memory_pool, const char *string);

    void *memory_pool;
}sorm_allocator_t;

typedef struct {
    const sorm_connection_t *conn;
    int tables_num;
    int has_more;
    const sorm_table_descriptor_t **tables_desc;
    select_columns_t *select_columns_of_tables;
    sqlite3_stmt *stmt_handle;
    const sorm_allocator_t *allocator;
}sorm_iterator_t;

static const char* sorm_errorstr[] = 
{
    "There is no error",        /* 0 - SORM_OK */
    "Can not allocate memory",    /* 1 - SORM_NOMEM */
    "Invalid Columns name or filter, or Sqlite3 internal error", 
    /* 2 = SORM_DB_ERROR */
    "Too long sqlite3 statment",    /* 3 - SOMR_TOO_LONG */
    "There is no Primary key",        /* 4 - SORM_NOPK */
    "Rows to be selected do not exist",  /* 5 - SORM_NOEXIST */
    "The number of rows to be selected is invalid", 
    /* 6 - SORM_INVALID_NUM */
    "Invalid column name",        /* 7 - SORM_INVALID_COLUMN_NAME */
    "Invalid sorm_mem",            /* 8 - SORM_INVALID_MEM */
    "Invalid sorm_type",        /* 9 - SORM_INVALID_TYPE */
    "One or more arguments is NULL",    /* 10 - SORM_ARG_NULL */
    "Param columns_name when calling select function is an empty string", 
    /* 11 - SORM_COLUMNS_NAME_EMPTY */
    "Param filter when calling select function is an empty string", 
    /* 12 - SORM_FILTER_EMPTY */
    "Sorm intialize fail",    /* 13 - SORM_INIT_FAIL / */
    "String buffer is not big enough when transfer a sorm object to string",
    /* 14 - SORM_STRING_BUF_NOT_ENOUGH */
    NULL
};

static inline const char* sorm_strerror(int error_code)
{
    return sorm_errorstr[error_code];
}

static inline const char* sorm_db_strerror(
        const sorm_connection_t *conn)
{
    return sqlite3_errmsg(conn->sqlite3_handle);
}

static const char* sorm_joinstr[] =
{
    "SORM_INNER_JOIN",
    "SORM_LEFT_JOIN",
};

static inline const char* sorm_strjoin(sorm_join_t join)
{
    return sorm_joinstr[join];
}

static const char* sorm_typestr[] =
{
    "SORM_TYPE_INT",
    "SORM_TYPE_INT64",
    "SORM_TYPE_TEXT",
    "SORM_TYPE_DOUBLE",
    "SORM_TYPE_BLOB",
};

static inline const char* sorm_strtype(sorm_type_t type)
{
    return sorm_typestr[type];
}

static const char* sorm_memstr[] =
{
    "SORM_MEM_NONE",
    "SORM_MEM_HEAP",
    "SORM_MEM_STACK",
};

static inline const char* sorm_strmem(sorm_mem_t mem)
{
    return sorm_memstr[mem];
}

static const char* sorm_constraintstr[] =
{
    "SORM_CONSTRAINT_NONE",
    "SORM_CONSTRAINT_PK",
    "SORM_CONSTRAINT_PK_ASC",
    "SORM_CONSTRAINT_PK_DESC",
    "SORM_CONSTRAINT_UNIQUE",
};

static inline const char* sorm_strconstraint(sorm_constraint_t constraint)
{
    return sorm_constraintstr[constraint];
}

/**
 * @brief: set the allocator 
 *
 * @param memory_pool: pointer to the memory pool used in allocator
 * @param alloc: pointer to the alloc function
 * @param free: pointer to the free function
 * @param strdup: pointer to the strdup function
 */
void sorm_set_allocator(void *memory_pool, 
        void*(*alloc)(void *memory_pool, size_t size), 
        void(*free)(void *memory_pool, void *point), 
        char*(*strdup)(void *memory_pool, const char *string));

int sorm_init();
void sorm_final();

char* sorm_to_string(const sorm_table_descriptor_t *table_desc,
        char *string, int len);

/**
 * @brief: for the string with single quote, replace the single 
 * quote with two single quote. the len is used to prevent from
 * overflow, string and fixed_string can point to the same buffer
 *
 * @param string: 
 * @param fixed_string:
 * @param len: the length of the buffer to store the fixed string
 *
 * @return: SORM_OK; SORM_TOO_LONG
 */
int sorm_fix_string(
        const char *string, char *fixed_string, int len);

/**
 * @brief: create a connection to a database
 *
 * @param path: database file path
 * @param db_type: database type
 * @param sem_key
 * @param flags: can be SORM_ENABLE_FOREIGIN_KEY | SORM_ENABLE_SEMAPHORE
 * @param connection: pointer to where stored the new connection
 *
 * @return: SORM_OK; SORM_ARG_NULL, SORM_NOMEM, SORM_DB_ERROR
 */
int sorm_open(
        const char *path, sorm_db_t db, int sem_key,
        pthread_rwlock_t *rwlock, int flags, 
        sorm_connection_t **connection);

/**
 * @brief: close the connection to a database
 *
 * @param conn: the to be closed connection
 *
 * @return: error code
 */
int sorm_close(sorm_connection_t *conn);

/**
 * @brief: run a user define database statement
 *
 * @param conn: pointer to the database connection
 * @param sql_stmt: pointer to the user defined database statement
 *
 * @return: error code
 */
int sorm_run_stmt(
        const sorm_connection_t *conn, char *sql_stmt, 
        int rwlock_flag);
/**
 * @brief: create a table in a database according the table's descriptor
 *
 * @param conn: pointer to the database connection
 * @param table_desc: pointer to the table descriptor
 *
 * @return: SORM_OK; SORM_ARG_NULL, SORM_TOO_LONG, SORM_DB_ERROR
 */
int sorm_create_table(
        const sorm_connection_t *conn, const sorm_table_descriptor_t *table_desc);
/**
 * @brief: delete a table in a database according to the tables's descriptor
 *
 * @param conn: pointer to the database connection
 * @param table_desc: pointer to the table descriptor
 *
 * @return: SORM_OK; SORM_ARG_NULL, SORM_TOO_LONG, SORM_DB_ERROR
 */
int sorm_delete_table(
        const sorm_connection_t *conn, const sorm_table_descriptor_t *table_desc);
/**
 * @brief: new a sorm object for a table
 *
 * @param init_table_desc: pointer to the table description
 *
 * @return: pointer to the new sorm object, NULL if fail
 */
sorm_table_descriptor_t* sorm_new(
        const sorm_allocator_t *allocator, 
        const sorm_table_descriptor_t *init_table_desc);
/**
 * @brief: new an array of sorm objects according the table descriptor
 *
 * @param init_table_desc: pointer to the table descriptor
 * @param num: the lenght of the array
 *
 * @return: pointer the the array, NULL if fail
 */
sorm_table_descriptor_t * sorm_new_array(
        const sorm_allocator_t *allocator, 
        const sorm_table_descriptor_t *init_table_desc, int n);
/**q
 * @brief: free the momery for the sorm object
 *
 * @param table_desc: the pointer to the sorm object
 */
void sorm_free(const sorm_allocator_t *allocator, 
        sorm_table_descriptor_t *table_desc);
void sorm_free_array(const sorm_allocator_t *allocator, 
        sorm_table_descriptor_t *table_desc, int n);
/**
 * @brief: set a column's value for a sorm object
 *
 * @param table_desc: pointer to the sorm object
 * @param column_index: the index of the column which is to be set a value
 * @param value: pointer to the value that is to be set to the column
 * @param value_len : only used for SOMR_TYPE_BLOB
 *
 * @return: SORM_OK; SORM_ARG_NULL, SORM_INVALID_MEM, SORM_INVALID_TYPE, SORM_NOMEM
 */
int sorm_set_column_value(
        sorm_table_descriptor_t *table_desc, 
        int column_index, const void *value, int value_len);
/**
 * @brief: insert a new row, if exists, the insert will fail.
 *
 * @param desc: pointer to data of the row , which has table description in the head
 *
 * @return: SORM_OK; SORM_ARG_NULL, SORM_TOO_LONG, SORM_DB_ERROR, 
 */
int sorm_insert(const sorm_connection_t *conn, sorm_table_descriptor_t *desc);
/**
 * @brief: replace a row. if exists, delete the existing row and insert a new one. if not exists, insert a new one.
 *
 * @param desc: the descriptor for the table, and the data about the row
 *
 * @return: error code
 */
int sorm_replace(const sorm_connection_t *conn, sorm_table_descriptor_t *desc);
/**
 * @brief: delete a row form a table
 *
 * @param desc: the descriptor for the table, and the data about the row
 *
 * @return: error code
 */
int sorm_delete(const sorm_connection_t *conn, sorm_table_descriptor_t *desc);
/**
 * @brief: delete a row from a table according to the primary key
 *
 * @param conn: the connection to the database
 * @param table_desc: the table description for the table
 * @param PK_val: the Primary key
 *
 * @return: error code
 */
int sorm_delete_by_column(
        const sorm_connection_t *conn, const sorm_table_descriptor_t *table_desc,
        int column_index, const void *column_value);
/**
 * @brief: delete rows from a table according the filter, the filter use
 *    the same syntax with statement after WHERE in SQL
 *
 * @param conn: the connection to the database
 * @param desc: the description for the table
 * @param filter: the filter
 *
 * @return: error code
 */
int sorm_delete_by(
        const sorm_connection_t *conn, const sorm_table_descriptor_t *table_desc, 
        const char *filter);
int sorm_select_one_by_column(
        const sorm_connection_t *conn, 
        const sorm_allocator_t *allocator, 
        const sorm_table_descriptor_t *table_desc,
        const char *columns_name, int column_index, 
        const void *column_value,
        sorm_table_descriptor_t **get_row);
int sorm_select_some_array_by(
        const sorm_connection_t *conn, 
        const sorm_allocator_t *allocator, 
        const sorm_table_descriptor_t *desc,
        const char *columns_name, const char *filter, 
        int *n, sorm_table_descriptor_t **get_row);
int sorm_select_some_list_by(
        const sorm_connection_t *conn, 
        const sorm_allocator_t *allocator, 
        const sorm_table_descriptor_t *desc,
        const char *column_names, const char *filter, 
        int *n, sorm_list_t **get_row_head);
int sorm_select_all_array_by(
        const sorm_connection_t *conn, 
        const sorm_allocator_t *allocator, 
        const sorm_table_descriptor_t *desc,
        const char *column_names, const char *filter, 
        int *n, sorm_table_descriptor_t **get_row);
int sorm_select_all_list_by(
        const sorm_connection_t *conn, 
        const sorm_allocator_t *allocator, 
        const sorm_table_descriptor_t *desc,
        const char *column_names, const char *filter, 
        int *n, sorm_list_t **get_row_head);
/**
 * @brief: begin a transaction in a connection, do not support nest
 *
 * @param conn: the connection
 *
 * @return: error code
 */
int sorm_begin_write_transaction(sorm_connection_t *conn);
int sorm_begin_read_transaction(sorm_connection_t *conn);
/**
 * @brief: commit a transaction in a connection, do not support nest
 *
 * @param conn: the connection
 *
 * @return: error code
 */
int sorm_commit_transaction(sorm_connection_t *conn);
/**
 * @brief: rollback a transaction in a connection, do not support nest
 *
 * @param conn: the connection
 *
 * @return: error code
 */
int sorm_rollback_transaction(sorm_connection_t *conn);
/**
 * @brief: join two table, and select for the joined table, return a result of array
 *
 * @param conn: the database connection
 * @param table1_desc: the description for table1
 * @param table1_column_name: the name of column in table1 for join
 * @param table2_desc: the description for table2
 * @param table2_column_name: the name of column in table2 for join
 * @param join: join type, can be SORM_INNER_JOIN, SORM_LEFT_JOIN,
 SORM_RIGHT_JOIN, SORM_FULL_JOIN
 * @param filter: filter for select, the same syntax with SQL
 * @param n: the number of selected rows
 * @param table1_row_head: the list of selected table1's row
 * @param table2_row_head: the list of selected table2's row
 *
 * @return: error code
 */
int sorm_select_some_array_by_join(
        const sorm_connection_t *conn, 
        const sorm_allocator_t *allocator, 
        const char *columns_name,
        const sorm_table_descriptor_t *table1_desc, 
        const char *table1_column_name,
        const sorm_table_descriptor_t *table2_desc, 
        const char *table2_column_name,
        sorm_join_t join, const char *filter, int *n,
        sorm_table_descriptor_t **table1_rows, 
        sorm_table_descriptor_t **table2_rows);
/**
 * @brief: join two table, and select for the joined table, return a result of list
 *
 * @param conn: the database connection
 * @param table1_desc: the description for table1
 * @param table1_column_name: the name of column in table1 for join
 * @param table2_desc: the description for table2
 * @param table2_column_name: the name of column in table2 for join
 * @param join: join type, can be SORM_INNER_JOIN, SORM_LEFT_JOIN,
 SORM_RIGHT_JOIN, SORM_FULL_JOIN
 * @param filter: filter for select, the same syntax with SQL
 * @param n: the number of selected rows
 * @param table1_row_head: the list of selected table1's row
 * @param table2_row_head: the list of selected table2's row
 *
 * @return: error code
 */
int sorm_select_some_list_by_join(
        const sorm_connection_t *conn, 
        const sorm_allocator_t *allocator, 
        const char *columns_name,
        const sorm_table_descriptor_t *table1_desc, 
        const char *table1_column_name,
        const sorm_table_descriptor_t *table2_desc, const 
        char *table2_column_name,
        sorm_join_t join, const char *filter, int *n,
        sorm_list_t **table1_row_head, 
        sorm_list_t **table2_row_head);
int sorm_select_all_array_by_join(
        const sorm_connection_t *conn, 
        const sorm_allocator_t *allocator, 
        const char *columns_name,
        const sorm_table_descriptor_t *table1_desc, 
        const char *table1_column_name,
        const sorm_table_descriptor_t *table2_desc, 
        const char *table2_column_name,
        sorm_join_t join, const char *filter, int *n,
        sorm_table_descriptor_t **table1_rows, 
        sorm_table_descriptor_t **table2_rows);
int sorm_select_all_list_by_join(
        const sorm_connection_t *conn,
        const sorm_allocator_t *allocator, 
        const char *columns_name, 
        const sorm_table_descriptor_t *table1_desc, 
        const char *table1_column_name,
        const sorm_table_descriptor_t *table2_desc, 
        const char *table2_column_name,
        sorm_join_t join, const char *filter, int *n,
        sorm_list_t **table1_row_head, sorm_list_t **table2_row_head);
int sorm_create_index(
        const sorm_connection_t *conn, 
        sorm_table_descriptor_t *table_desc, char *columns_name);
int sorm_drop_index(
        const sorm_connection_t *conn,
        char *columns_name);
int sorm_update(
        const sorm_connection_t *conn, 
        sorm_table_descriptor_t *table_desc);

int sorm_select_iterate_by_join_open(
        const sorm_connection_t *conn, 
        const sorm_allocator_t *allocator, 
        const char *columns_name,
        const sorm_table_descriptor_t *table1_desc, 
        const char *table1_column_name,
        const sorm_table_descriptor_t *table2_desc, 
        const char *table2_column_name,
        sorm_join_t join, const char *filter, 
        sorm_iterator_t **iterator);

int sorm_select_iterate_by_open( 
        const sorm_connection_t *conn, 
        const sorm_allocator_t *allocator, 
        const sorm_table_descriptor_t *table_desc, 
        const char *columns_name, const char *filter, 
        sorm_iterator_t **iterator);

int sorm_select_iterate_by_join(
        sorm_iterator_t *iterator, 
        sorm_table_descriptor_t **table1_row, 
        sorm_table_descriptor_t **table2_row);

int sorm_select_iterate_by(
        sorm_iterator_t *iterator, 
        sorm_table_descriptor_t **table_row);

int sorm_select_iterate_close(sorm_iterator_t *iterator);
/**
 * @brief: 1 means has more, else return 0
 */
int sorm_select_iterate_more(sorm_iterator_t *iterator);
/**
 * @brief: return the number of database rows that were changed
 * or inserted or deleted by the most recently completed SQL 
 * statement
 */
int sorm_changes(sorm_connection_t *conn);

#endif
