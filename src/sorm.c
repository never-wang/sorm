/****************************************************************************
 *       Filename:  sorm.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/11/13 12:40:28
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sorm.h"
#include "memory.h"
#include "log.h"
#include "semaphore.h"

#define _list_cpy_free(desc, head, n, rows, free) \
    __list_cpy_free(desc, head, n, rows, (void*)(free))
#define _heap_member_pointer(table_desc, member_offset) \
     *(char **)((char*)(table_desc) + (member_offset))
#define _stack_member_pointer(table_desc, member_offset) \
    ((char*)(table_desc) + (member_offset))
#define _type_member_pointer(table_desc, offset, type) \
    ((type*)((char*)(table_desc) + (offset)))
    

/** @brief: type to string which is used in sql statement */
static const char* sorm_type_db_str[] = 
{
    "INTEGER",	    /* 0 - SORM_TYPE_INT32 */
    "TEXT",	    /* 1 - SORM_TYPE_TEXT */
    "REAL",	    /* 2 - SORM_TYPE_DOUBLE */
    "BLOB",	    /* 3 - SORM_TYPE_BLOB */
};

/** @brief: used to store the information for selected columns in a table */
typedef struct
{
    int columns_num;	    /* the number of selected columns */
    int *indexes_in_result;  /* the index of selected columns in result */
}select_columns_t;

typedef int(*select_core_t)(const sorm_connection_t*, sqlite3_stmt*, 
        int, const sorm_table_descriptor_t**, const int*, const select_columns_t*,
        int, int *, void **);

void _list_free(sorm_list_t *sorm_list, void (*data_free)(void*))
{
    sorm_list_t *pos, *pre;

    //log_debug("Start.");
    if(sorm_list != NULL)
    {
        sorm_list_for_each_safe(pos, pre, sorm_list)
        {
            if(data_free != NULL)
            {
                data_free(pos->data);
            }
            mem_free(pos);
        }
        mem_free(sorm_list);
    }
    //log_debug("Success return.");
}

/**
 * @brief: copy data in a list into an array of rows, and free the list
 */
static inline void __list_cpy_free(
        const sorm_table_descriptor_t *table_desc,
        sorm_list_t *sorm_list, int n, sorm_table_descriptor_t *rows,
        void(*data_free)(void*))
{
    int i = 0;
    sorm_list_t *pos, *scratch;

    assert(sorm_list != NULL);
    assert(rows!= NULL);

    //log_debug("Start.");

    sorm_list_for_each_safe(pos, scratch, sorm_list)
    {
        log_debug("List get(%d)", i);
        memcpy((char *)rows + table_desc->size * i, pos->data, table_desc->size);
        i ++;
        if(data_free != NULL)
        {
            data_free(pos->data);
        }
        mem_free(pos);
    }

    assert(i == n);
    mem_free(sorm_list);
    //log_debug("Success return");
}

static inline int _get_column_stat(
        const sorm_table_descriptor_t *table_desc, int column_index)
{
    assert(table_desc != NULL);
    assert((column_index >= 0) && (column_index < table_desc->columns_num));

    return (*(sorm_stat_t *)((char*)table_desc + 
                table_desc->columns[column_index].offset 
                - sizeof(sorm_stat_t)));  
}

static inline void _set_column_stat(
        sorm_table_descriptor_t *table_desc, int column_index, sorm_stat_t stat)
{
    assert(table_desc != NULL);
    assert((column_index >= 0) && (column_index < table_desc->columns_num));

    (*(sorm_stat_t *)((char*)table_desc + 
                      table_desc->columns[column_index].offset 
                      - sizeof(sorm_stat_t))) = stat;
}

static inline void _add_column_stat(
        sorm_table_descriptor_t *table_desc, int column_index, 
        sorm_stat_t add_stat)
{
    assert(table_desc != NULL);
    assert((column_index >= 0) && (column_index < table_desc->columns_num));
   
    sorm_stat_t stat;
    stat = _get_column_stat(table_desc, column_index);
    stat |= add_stat;
    _set_column_stat(table_desc, column_index, stat);
}

static inline int _get_blob_len(
	const sorm_table_descriptor_t *table_desc, int column_index)
{
    assert(table_desc != NULL);
    assert((column_index >= 0) && (column_index < table_desc->columns_num));

    return (*(int*)((char*)table_desc + table_desc->columns[column_index].offset
		-sizeof(sorm_stat_t) - sizeof(int)));
}

static inline int _set_blob_len(
	sorm_table_descriptor_t *table_desc, int column_index, int len)
{
    assert(table_desc != NULL);
    assert((column_index >= 0) && (column_index < table_desc->columns_num));

    (*(int*)((char*)table_desc + table_desc->columns[column_index].offset
	     -sizeof(sorm_stat_t) - sizeof(int))) = len;
}
/**
 * @brief: call sqlite3_bind to bind a column value into a prepared statement
 *
 * @param conn : the connection to the database
 * @param column_desc: the descriptor for the column
 * @param index: the index for the bind place in the prepared statment, start 
 *	from 1
 * @param table_desc: the descriptor for the table
 * @param stmt_handle: the prepared statement
 *
 * @return: SORM_OK; SORM_INVALID_MEM, SORM_INVALID_TYPE, SORM_DB_ERROR
 */
static int _sqlite3_column_bind(
        const sorm_connection_t *conn, sqlite3_stmt *stmt_handle,
        const sorm_table_descriptor_t *table_desc,
        int column_index, int bind_index) 
{
    int ret;

    assert(conn != NULL);
    assert(stmt_handle != NULL);
    assert(table_desc != NULL);
    assert((column_index >= 0) && (column_index < table_desc->columns_num));
    assert(sorm_is_stat_valued(_get_column_stat(table_desc, column_index)));

    const sorm_column_descriptor_t *column_desc = 
        &table_desc->columns[column_index];

    switch(column_desc->type)
    {
        case SORM_TYPE_INT :
            ret = sqlite3_bind_int(stmt_handle, bind_index, 
                    *((int32_t*)((char*)table_desc + column_desc->offset)));
            break;
            //TODO TEXT16
        case SORM_TYPE_TEXT :
            switch(column_desc->mem)
            {
                case SORM_MEM_HEAP :
                    ret = sqlite3_bind_text(stmt_handle, bind_index, 
                            *((char**)((char*)table_desc + column_desc->offset)), 
                            -1, SQLITE_STATIC);
                    break;
                case SORM_MEM_STACK :
                    ret = sqlite3_bind_text(stmt_handle, bind_index, 
                            (char*)table_desc + column_desc->offset, 
                            -1, SQLITE_STATIC);
                    break;
                default :
                    log_error("unknow sorm mem : %d", column_desc->mem);
                    return SORM_INVALID_MEM;
            }
            break;
        case SORM_TYPE_DOUBLE :
            ret = sqlite3_bind_double(stmt_handle, bind_index,
                    *((double*)((char*)table_desc + column_desc->offset)));
            break;
        case SORM_TYPE_BLOB :
            switch(column_desc->mem)
            {
                case SORM_MEM_HEAP :
                    ret = sqlite3_bind_blob(stmt_handle, bind_index, 
                            *((void**)((char*)table_desc + column_desc->offset)),  
                            _get_blob_len(table_desc, column_index), 
                            SQLITE_STATIC);
                    break;
                case SORM_MEM_STACK :
                    ret = sqlite3_bind_blob(stmt_handle, bind_index, 
                            (void*)((char*)table_desc + column_desc->offset),  
                            _get_blob_len(table_desc, column_index), 
                            SQLITE_STATIC);
                    break;
                default :
                    log_error("unknow sorm mem : %d", column_desc->mem);
                    return SORM_INVALID_MEM;
            }
            break;
        default :
            log_error("unknow sorm type : %d", column_desc->type);
            return SORM_INVALID_TYPE;
    }
    if(ret != SQLITE_OK)
    {
        log_debug("sqlite3_bind (%s) with index(%d) error : %s", column_desc->name,
                column_index, sqlite3_errmsg(conn->sqlite3_handle));
        return SORM_DB_ERROR;
    }

    //log_debug("Success return");
    return SORM_OK;
}

/**
 * @brief: trim a str, use * to expree blank, then a string
 *	|			      |
 *	V			      V	
 *	***string**\0  will becom  ***string\0\0\0
 *
 * @param str: pointer to the string
 * @param str_end: pointer the end of string, if NULL, the function will find 
 *	the end by itself
 */
static inline void trim(char **str_p, char *str_end)
{
    char *str = *str_p;

    //log_debug("Start.");

    /* trim head */
    while(((*str) == ' ') || ((*str) == '\t'))
    {
        str ++;
    }
    *str_p = str;

    /* find end */
    if(str_end == NULL)
    {
        str_end = str;
        while((*str_end) != '\0') 
        {
            str_end ++;
        }
    }

    /* trim tail */
    str_end --;
    while(((*str_end) == ' ') || ((*str_end) == '\t'))
    {
        *str_end = '\0';
        str_end --;
    }

    //log_debug("Success return");
}
/**
 * @brief: call sqlite3_column to get a column value of a row
 *
 * @param column_desc: 
 * @param index: the index of the column, start from 0
 * @param get_row: the got row where store the got value
 * @param stmt_handle: the sqlit3 result
 *
 * @return:  error code
 */
static inline int _sqlite3_column(
        sorm_table_descriptor_t *table_desc, sqlite3_stmt *stmt_handle,
        int column_index, int result_index)
{
    int ret, offset;
    char *text = NULL;
    void *blob_db = NULL, *blob = NULL;
    const sorm_column_descriptor_t *column_desc = NULL;
    int blob_len;

    assert(table_desc != NULL);
    assert(stmt_handle != NULL);

    //log_debug("Start.");
    column_desc = &table_desc->columns[column_index];

    switch(column_desc->type)
    {
        case SORM_TYPE_INT :
            *_type_member_pointer(table_desc, column_desc->offset, int32_t) =
                sqlite3_column_int(stmt_handle, result_index);
            break;
            //TODO TEXT16
        case SORM_TYPE_TEXT :
            if(column_desc->mem == SORM_MEM_HEAP)
            {
                text = mem_strdup((char *)sqlite3_column_text(
                            stmt_handle, result_index));
                _heap_member_pointer(table_desc, column_desc->offset) = text; 
                _add_column_stat(table_desc, column_index, 
                        SORM_STAT_NEEDFREE);
            }else if(column_desc->mem == SORM_MEM_STACK)
            {
                text = (char *)sqlite3_column_blob(
                        stmt_handle, result_index);
                if(text != NULL)
                {
                    text = (char *)sqlite3_column_text(
                            stmt_handle, result_index);
                    ret = snprintf(_stack_member_pointer(table_desc, 
                                column_desc->offset), 
                                column_desc->max_len + 1, "%s", text);
                            offset = ret;
                            if(ret < 0 || offset > column_desc->max_len)
                            {
                            log_error("get too long text from db, "
                                "length(%d) > max length(%d)", offset, 
                                column_desc->max_len);
                        return SORM_TOO_LONG;
                    }
                }
            }else
            {
                log_error("unknow sorm mem : %d", column_desc->mem);
                return SORM_INVALID_MEM;
            }
            break;
        case SORM_TYPE_DOUBLE :
            *_type_member_pointer(table_desc, column_desc->offset, double) = 
                sqlite3_column_double(stmt_handle, result_index);
            break;
        case SORM_TYPE_BLOB :
            if(column_desc->mem == SORM_MEM_HEAP)
            {
                blob_db = sqlite3_column_blob(stmt_handle, result_index);
                blob_len = sqlite3_column_bytes(stmt_handle, result_index);
                if(blob_len != 0)
                {
                    blob = mem_malloc(blob_len);
                    if(blob == NULL)
                    {
                        log_error("malloc for blob fail");
                        return SORM_NOMEM;
                    }
                    memcpy(blob, blob_db, blob_len);
                _add_column_stat(table_desc, column_index, 
                        SORM_STAT_NEEDFREE);
                }else
                {
                    blob = NULL;
                }
                _heap_member_pointer(table_desc, column_desc->offset) = blob;
            }else if(column_desc->mem == SORM_MEM_STACK)
            {
                text = (char *)sqlite3_column_blob(stmt_handle, result_index);
                blob_len = sqlite3_column_bytes(stmt_handle, result_index);

                if(blob_len > column_desc->max_len)
                {
                        log_error("get too long blob from db, "
                                "length(%d) > max length(%d)", blob_len, 
                                column_desc->max_len);
                        return SORM_TOO_LONG;
                }
                memcpy(_stack_member_pointer(table_desc, 
                            column_desc->offset), text, blob_len); 
            }else
            {
                log_error("unknow sorm mem : %d", column_desc->mem);
                return SORM_INVALID_MEM;
            }
	        _set_blob_len(table_desc, column_index, blob_len);
            break;
        default :
            log_error("unknow sorm type : %d", column_desc->type);
            return SORM_INVALID_TYPE;
    }

    _add_column_stat(table_desc, column_index, SORM_STAT_VALUED);
    //log_debug("Success return");
    return SORM_OK;
}

static inline int  _sqlite3_step(
        const sorm_connection_t *conn, sqlite3_stmt *stmt_handle)
{
    int ret;

    if((conn->transaction_num == 0) && 
	    (sorm_semaphore_enabled(conn->flags) == 1)) /* no transaction */
    {
        sem_p(conn->sem_key);
    }


    ret = sqlite3_step(stmt_handle);
    
    if((conn->transaction_num == 0) && 
	    (sorm_semaphore_enabled(conn->flags) == 1)) /* no transaction */
    {
        sem_v(conn->sem_key);
    }

    return ret;
}

static inline int _sqlite3_prepare(
        const sorm_connection_t *conn, char *sql_stmt, sqlite3_stmt **stmt_handle)
{
    int ret;
    assert(conn != NULL);
    
    if((conn->transaction_num == 0) && 
	    (sorm_semaphore_enabled(conn->flags) == 1)) /* no transaction */
    {
        sem_p(conn->sem_key);
    }


    ret = sqlite3_prepare(conn->sqlite3_handle, sql_stmt, SQL_STMT_MAX_LEN,
            stmt_handle, NULL);
    
    if((conn->transaction_num == 0) && 
	    (sorm_semaphore_enabled(conn->flags) == 1)) /* no transaction */
    {
        sem_v(conn->sem_key);
    }

    return ret;
}

/**
 * @brief: get the columns number in a columns_name, the function need the 
 *	description for tables to caculate the colums number for "*"
 *
 * @param tables_num: 
 * @param tables_desc: 
 * @param columns_name: 
 * @param columns_num: the got columns number
 *
 * @return: error code
 */
static inline int _get_number_from_columns_name(
        int tables_num,
        const sorm_table_descriptor_t *tables_desc,
        char *columns_name, int *columns_num)
{
    char* delimiter;
    char *column_name = NULL, *_columns_name = NULL;
    char *_table_name = NULL, *_column_name = NULL;
    int i;

    assert(tables_desc != NULL);
    assert(columns_name != NULL);
    assert(columns_num != NULL);

    column_name = _columns_name = mem_strdup(columns_name);
    //column_name used to find column_name, _columns_name used for free
    if(column_name == NULL)
    {
        log_debug("mem_strdup for column_name fail");
        return;
    }

    for(i = 0; i < tables_num; i ++)
    {
        columns_num[i] = 0;
    }

    do
    {
        /* get each column_name */
        delimiter = strchr(column_name, COLUMN_NAMES_DELIMITER);
        if(delimiter != NULL)
        {
            *delimiter = '\0';
        }
        trim(&column_name, delimiter); 

        if((_column_name = strchr(column_name, TABLE_COLUMN_DELIMITER)) != NULL)
            // column_name = _device_name._column_name
        {
            *_column_name = '\0';
            _column_name ++;
            _table_name = column_name;
        }else // column_name = _column_name
        {
            _column_name = column_name;
        }

        if(strcmp(_column_name, ALL_COLUMNS) == 0)
            // _column_name = * 
        {
            if(_table_name == NULL) // oolumn_name = *
            {
                for(i = 0; i < tables_num; i ++)
                {
                    columns_num[i] = tables_desc[i].columns_num;
                }
            }else		    // column_name = _device_name.*
            {
                for(i = 0; i < tables_num; i ++)
                {
                    if(strcmp(_table_name, tables_desc[i].name) == 0)
                    {
                        columns_num[i] = tables_desc[i].columns_num;
                        break;
                    }
                }
                assert(0);
            }
        }else 
        {
            if(_table_name == NULL) // oolumn_name = _column_name
            {
                for(i = 0; i < tables_num; i ++)
                {
                    columns_num[i] = tables_desc[i].columns_num;
                }
            }else		    // column_name = _device_name.*
            {
                for(i = 0; i < tables_num; i ++)
                {
                    if(strcmp(_table_name, tables_desc[i].name) == 0)
                    {
                        columns_num[i] = tables_desc[i].columns_num;
                        break;
                    }
                }
                assert(0);
            }
            (*columns_num) ++;
        }

        if(delimiter != NULL)
        {
            column_name = delimiter + 1;
        }

    }while(delimiter != NULL);

    mem_free(_columns_name);

    for(i = 0; i < tables_num; i ++)
    {
        log_debug("select %d columns from table %s", 
                columns_num[i], tables_desc[i].name);
    }

    return SORM_OK;
}

/**
 * @brief: give a column_name and some tables, find the column_name is in which
 *	table and in wich column of the table
 *
 * @param tables_num:
 * @param tables_desc: 
 * @param column_name: the name of column, can be column_name or 
 *	device_name.column_name; 
 *	the input string may be changed, like:
 *	device.column\0 change to device\0column\0
 * @param table_index: the index of table 
 * @param column_index: the index of column, 
 *	an special vall is ALL_INDEXES for "*"
 *
 * @return: error code
 */
static inline int _column_name_to_index_in_tables(
        int tables_num, const sorm_table_descriptor_t **tables_desc,
        char *column_name,
        int *table_index, int *column_index)
{
    int i, j;
    char *_table_name = NULL, *_column_name = NULL;

    assert(tables_desc != NULL);
    assert(column_name != NULL);
    assert(index != NULL);

    //log_debug("Start.");
    log_debug("Parse column_name : %s", column_name);

    if((_column_name = strchr(column_name, TABLE_COLUMN_DELIMITER)) != NULL)
    {
        *_column_name = '\0';
        _column_name ++;
        _table_name = column_name;
    }else
    {
        _column_name = column_name;
    }

    /* check the table_name is valid */
    (*table_index) = -1;
    if(_table_name != NULL) // the name is "colomn"
    {
        for(i = 0; i < tables_num; i ++)
        {
            if(strcmp(_table_name, tables_desc[i]->name) == 0)
            {
                (*table_index) = i;
                break;
            }
        }
        if((*table_index) == -1)
        {
            log_debug("Invalid table name in column name.");
            return SORM_INVALID_COLUMN_NAME;
        }
    }

    /* check the column_name is valid */
    if(strcmp(_column_name, ALL_COLUMNS) == 0) // _device_name.* or *
    {

        if((*table_index) == -1) // *
        {
            (*table_index) = ALL_INDEXES;
        }

        (*column_index) = ALL_INDEXES;

        log_debug("talbe_index = %d, column_index = ALL_INDEXES",
                (*table_index));

        return SORM_OK;
    }else
    {
        if((*table_index) == -1) //the name is "colomn", find every table
        {
            for(i = 0; i < tables_num; i ++)
            {
                for(j = 0; j < tables_desc[i]->columns_num; j ++)
                {
                    if(strcmp(tables_desc[i]->columns[j].name, _column_name) == 0)
                    {
                        (*column_index) = j;
                        (*table_index) = i;
                        log_debug("table_index = %d, column_index = %d",
                                (*table_index), (*column_index));
                        return SORM_OK;
                    }
                }
            }
        }else 
        {
            for(i = 0; i < tables_desc[*table_index]->columns_num; i ++)
            {
                if(strcmp(tables_desc[*table_index]->columns[i].name, 
                            _column_name) == 0)
                {
                    (*column_index) = i;
                    log_debug("table_index = %d, column_index = %d",
                            (*table_index), (*column_index));
                    //log_debug("Success return.");
                    return SORM_OK;
                }
            }
        }
    }

    assert(0);
}

/**
 * @brief: convert columns_name to a struct select_columns_t, 
 *	the struct select_columns_t has three member
 *	1. the number of columns
 *	1. the column indexes in table 
 *	2. the column indexes in result
 *	columns_name can be from multiple tables
 *
 * @param table_num: the number of tables 
 * @param table_desc: 
 * @param columns_name: 
 * @param select_columns_of_table: 
 *
 * @return: 
 */
static inline int _columns_name_to_select_columns(
        int tables_num, const sorm_table_descriptor_t **tables_desc, 
        const char *columns_name, select_columns_t *select_columns_of_table)
{
    char* delimiter;
    char *column_name = NULL, *_columns_name = NULL;
    char **column_name_array = NULL;
    int i, j, 
        ret, _columns_num, result_columns_num, total_columns_num;
    int table_index, column_index;
    int ret_val;

    assert(tables_desc != NULL);
    assert(columns_name != NULL);
    assert(select_columns_of_table != NULL);

    //log_debug("Start.");
    //

    column_name = _columns_name = mem_strdup(columns_name);
    if(column_name == NULL)
    {
        log_debug("mem_strdup for columns_name fail");
        ret_val = SORM_NOMEM;
        goto RETURN;
    }

    for(i = 0; i < tables_num; i ++)
    {
        select_columns_of_table[i].columns_num = 0;
        for(j = 0; j < tables_desc[i]->columns_num; j ++)
        {
            select_columns_of_table[i].indexes_in_result[j] = 
                NO_INDEXES_IN_RESULT;
        }
    }

    /* split columns_name and store them in a list */
    result_columns_num = 0;

    do
    {
        delimiter = strchr(column_name, COLUMN_NAMES_DELIMITER);
        if(delimiter != NULL)
        {
            *delimiter = '\0';
        }
        trim(&column_name, delimiter); 


        ret = _column_name_to_index_in_tables(
                tables_num, tables_desc, column_name, 
                &table_index, &column_index);

        if(ret == SORM_OK)
        {
            if(column_index == ALL_INDEXES) /*once select *, refresh information*/
            {
                if(table_index == ALL_INDEXES) // *
                {
                    for(j = 0; j < tables_num; j ++)
                    {
                        select_columns_of_table[j].columns_num = 
                            tables_desc[j]->columns_num;
                        for(i = 0; i < tables_desc[j]->columns_num; i ++)
                        {
                            select_columns_of_table[j].indexes_in_result[i]
                                = result_columns_num;
                            result_columns_num ++;
                        }
                    }

                }else   //device_name.*
                {
                    select_columns_of_table[table_index].columns_num = 
                        tables_desc[table_index]->columns_num;
                    for(i = 0; i < tables_desc[table_index]->columns_num; i ++)
                    {
                        select_columns_of_table[table_index].indexes_in_result[i]
                            = result_columns_num;
                        result_columns_num ++;
                    }
                }
            }else
            {
                if(select_columns_of_table[table_index].
                        indexes_in_result[column_index] == NO_INDEXES_IN_RESULT)
                {
                    select_columns_of_table[table_index].
                        indexes_in_result[column_index] = result_columns_num;
                    select_columns_of_table[table_index].columns_num ++;
                }
                result_columns_num ++;
            }

        }

        if(delimiter != NULL)
        {
            column_name = delimiter + 1;
        }
    }while(delimiter != NULL);
    ret_val = SORM_OK;
    //log_debug("Success return.");

RETURN :
    if(_columns_name != NULL)
    {
        mem_free(_columns_name);
        _columns_name = NULL;
    }

    if(column_name_array != NULL)
    {
        mem_free(column_name_array);
        column_name_array = NULL;
    }

    return ret_val;
}

/**
 * @brief: parse a select result from a table to a sorm object, 
 *	and add it into a list tail
 * @param conn: the connection to the database
 * @param stmt_handle: the select result
 * @param row: the sorm object to store the parsed result
 * @param column_indexes_in_result: 
 *
 * @return: error code
 */
static inline int _parse_select_result(
        const sorm_connection_t *conn,
        sqlite3_stmt *stmt_handle,
        const int *column_indexes_in_result,
        sorm_table_descriptor_t *row)
{
    int i, ret;

    assert(conn != NULL);
    assert(row != NULL);
    assert(column_indexes_in_result != NULL);
    assert(row != NULL);

    //log_debug("Start.");
    /* get data of the row*/
    for(i = 0; i < row->columns_num; i ++)
    {
        if(column_indexes_in_result[i] != NO_INDEXES_IN_RESULT)
        {
            ret = _sqlite3_column(row, stmt_handle, 
                    i, column_indexes_in_result[i]); 
            if(ret != SORM_OK)
            {
                log_debug("_sqliter3_column error.");
                return ret;
            }
        }
    }
    //log_debug("Success return.");
    return SORM_OK;
}

/**
 * @brief: construct a filter for a column value
 *
 * @param table_desc: 
 * @param column_index: 
 * @param column_value
 * @param filter: 
 *
 * @return: error code
 */
static inline int _construct_column_filter(
        const sorm_table_descriptor_t *table_desc, int column_index,
        const void *column_value, char *filter)
{
    const sorm_column_descriptor_t *column_desc;
    int offset;

    log_debug("Start.");
    
    assert(table_desc != NULL);
    assert(column_value != NULL);
    assert(column_index < table_desc->columns_num);

    column_desc = &table_desc->columns[column_index];
    
    switch(column_desc->type)
    {
        case SORM_TYPE_INT :
            offset = snprintf(filter, SQL_STMT_MAX_LEN + 1, "%s = %d", 
                    column_desc->name, *((int32_t*)column_value));
            break;
        case SORM_TYPE_TEXT :
            offset = snprintf(filter, SQL_STMT_MAX_LEN + 1, "%s = %s", 
                    column_desc->name, (char*)column_value);
            break;
        case SORM_TYPE_DOUBLE :
            offset = snprintf(filter, SQL_STMT_MAX_LEN + 1, "%s = %f", 
                    column_desc->name, *((double*)column_value));
            break;
        default :
            log_debug("unknow sorm type : %d", column_desc->type);
            return SORM_INVALID_TYPE;
    }

    if(offset < 0 || offset > SQL_STMT_MAX_LEN)
    {
        log_debug("snprintf error while constructing filter, "
                "snprintf length(%d) > max length(%d)", offset, SQL_STMT_MAX_LEN);
        return SORM_TOO_LONG;
    }
    log_debug("End.");

    return SORM_OK;
}

/**
 * @brief: construct a select statement
 *
 * @param sql_stmt: pointer to thte statement
 * @param table_name: the name of table that is to be selected from
 * @param columns_name: the name of columns that are to be selected
 * @param offset: the end offset of statement
 *
 * @return: error code
 */
static inline int _construct_select_stmt(
        char *sql_stmt, 
        const char *table_name, const char *columns_name, int *offset)
{
    int ret;

    assert(sql_stmt != NULL);
    assert(table_name != NULL);
    assert(columns_name != NULL);
    assert(offset != NULL);

    //log_debug("Start.");

    (*offset) = ret = snprintf(sql_stmt, SQL_STMT_MAX_LEN + 1, 
            "SELECT %s FROM %s", columns_name, table_name);
    if(ret < 0 || (*offset) > SQL_STMT_MAX_LEN)
    {
        log_debug("snprintf error while constructing sql statment, "
                "snprintf length(%d) > max length(%d)", 
                (*offset), SQL_STMT_MAX_LEN);
        return SORM_TOO_LONG;
    }

    //log_debug("Success return.");
    return SORM_OK;
}

/**
 * @brief: construct a filter statement
 *
 * @param sql_stmt: pointer to thte statement
 * @param filter: 
 * @param offset: the end offset of statement
 *
 * @return: error code
 */
static inline int _construct_filter_stmt(
        char *sql_stmt, const char *filter, int *offset)
{
    int ret;

    //log_debug("Start.");

    if(filter != NULL)
    {
        ret = snprintf(sql_stmt + (*offset), SQL_STMT_MAX_LEN + 1 - (*offset), 
                " WHERE %s", filter);
        (*offset) += ret;
        if(ret < 0 || (*offset) > SQL_STMT_MAX_LEN)
        {
            log_debug("snprintf error while constructing sql statment, "
                    "snprintf length(%d) > max length(%d)", 
                    *offset, SQL_STMT_MAX_LEN);
            return SORM_TOO_LONG;
        }
    }

    //log_debug("Success return.");
    return SORM_OK;
}

void sorm_set_allocator(void *memory_pool, 
    void*(*_alloc)(void *memory_pool, size_t size), 
    void(*_free)(void *memory_pool, void *point), 
    char*(*_strdup)(void *memory_pool, const char *string))
{
    mem_set_allocator(memory_pool, _alloc, _free, _strdup);
}

int sorm_init()
{
    if(log_init() != LOG_OK)
    {
        printf("log init fail.\n");
        return SORM_INIT_FAIL;
    }

    //if(sem_init() != SEM_OK)
    //{
    //    error("semaphore init fail.");
    //    return SORM_INIT_FAIL;
    //}

    return SORM_OK;
}

void sorm_final()
{
    log_final();
    sem_final();
}

int sorm_open(
        const char *path, sorm_db_t db, int sem_key, int flags,
	sorm_connection_t **connection)
{
    sorm_connection_t *_connection = NULL;
    int ret, ret_val;
    sqlite3_stmt *stmt_handle = NULL;

    //log_debug("Start");

    if(path == NULL)
    {
        log_debug("Param path is NULL");
        return SORM_ARG_NULL;
    }

    log_debug("open db path(%s)", path);

    _connection = mem_malloc(sizeof(sorm_connection_t));
    if(_connection == NULL)
    {
        log_debug("mem_malloc for _connection fail");
        return SORM_NOMEM;
    }

    memset(_connection, 0, sizeof(sorm_connection_t));

    _connection->db_file_path = mem_strdup(path);
    if(_connection->db_file_path == NULL)
    {
        log_debug("mem_strdup for database file path fail");
        return SORM_NOMEM;
    }
    _connection->db = db;
    _connection->transaction_num = 0;

    ret = sqlite3_open(path, &_connection->sqlite3_handle);
    if(ret != SQLITE_OK)
    {
        log_debug("sqlite3_open fail");
        return SORM_DB_ERROR;
    }

    if((flags & SORM_ENABLE_FOREIGN_KEY) == SORM_ENABLE_FOREIGN_KEY)
    {
	ret = _sqlite3_prepare(_connection, "PRAGMA foreign_keys = ON", 
		&stmt_handle);

	if(ret != SQLITE_OK)
	{
	    log_debug("sqlite3_prepare error : %s", 
		    sqlite3_errmsg(_connection->sqlite3_handle));
	    return SORM_DB_ERROR;
	}

	ret = _sqlite3_step(_connection, stmt_handle);

	if(ret != SQLITE_DONE)
	{
	    log_debug("sqlite3_step error : %s", 
		    sqlite3_errmsg(_connection->sqlite3_handle));
	    ret_val = SORM_DB_ERROR;
	}else
	{
	    ret_val = SORM_OK;
	}
	ret = sqlite3_finalize(stmt_handle);
	if(ret != SQLITE_OK)
	{
	    log_debug("sqlite3_finalize error : %s", 
		    sqlite3_errmsg(_connection->sqlite3_handle));
	    return SORM_DB_ERROR;
	}
	if(ret_val != SORM_OK)
	{
	    return ret_val;
	}
    }

    if((flags & SORM_ENABLE_SEMAPHORE) == SORM_ENABLE_SEMAPHORE)
    {
	_connection->sem_key = sem_key;
    }
    _connection->flags = flags;
    
    *connection = _connection;

    return SORM_OK;
    /* foreign key support */

    //log_debug("Success return");
}

int sorm_create_table(
        const sorm_connection_t *conn, const sorm_table_descriptor_t *table_desc)
{
    char sql_stmt[SQL_STMT_MAX_LEN + 1] = "";
    sqlite3_stmt *stmt_handle = NULL;
    int offset;
    int ret, ret_val;
    int i;
    sorm_column_descriptor_t *column_desc;

    if(conn == NULL)
    {
        log_error("Param conn is NULL.");
        return SORM_ARG_NULL;
    }
    if(table_desc == NULL)
    {
        log_error("Param table_desc is NULL.");
        return SORM_ARG_NULL;
    }

    ret = offset = snprintf(sql_stmt, SQL_STMT_MAX_LEN + 1,
            "CREATE TABLE IF NOT EXISTS %s(", table_desc->name);
    if(ret < 0 || offset > SQL_STMT_MAX_LEN)
    {
        log_debug("snprintf error while constructing sql statment, "
                "snprintf length(%d) > max length(%d)", offset, SQL_STMT_MAX_LEN);
        return SORM_TOO_LONG;
    }

    for(i = 0; i < table_desc->columns_num; i ++)
    {
	column_desc = &(table_desc->columns[i]);
        ret = snprintf(sql_stmt + offset, SQL_STMT_MAX_LEN - offset + 1,
                " %s %s", column_desc->name, 
                sorm_type_db_str[column_desc->type]);
        offset += ret;
        if(ret < 0 || offset > SQL_STMT_MAX_LEN)
        {
            log_debug("snprintf error while constructing sql statment, "
                    "snprintf length(%d) > max length(%d)", 
                    offset, SQL_STMT_MAX_LEN);
            return SORM_TOO_LONG;
        }

        switch(column_desc->constraint)
        {
            case SORM_CONSTRAINT_PK :
                ret = snprintf(sql_stmt + offset, SQL_STMT_MAX_LEN - offset + 1,
                        " %s", "PRIMARY KEY,");
                break;
            case SORM_CONSTRAINT_PK_DESC :
                ret = snprintf(sql_stmt + offset, SQL_STMT_MAX_LEN - offset + 1,
                        " %s", "PRIMARY KEY DESC,");
                break;
            case SORM_CONSTRAINT_PK_ASC :
                ret = snprintf(sql_stmt + offset, SQL_STMT_MAX_LEN - offset + 1,
                        " %s", "PRIMARY KEY ASC,");
                break;
            case SORM_CONSTRAINT_UNIQUE :
                ret = snprintf(sql_stmt + offset, SQL_STMT_MAX_LEN - offset + 1,
                        " %s", "UNIQUE,");
                break;
            default :
                ret = snprintf(sql_stmt + offset, SQL_STMT_MAX_LEN - offset + 1, 
                        ",");
                break;
        }
        offset += ret;
        if(ret < 0 || offset > SQL_STMT_MAX_LEN)
        {
            log_debug("snprintf error while constructing sql "
                    "statment, snprintf length(%d) > max length(%d)", 
                    offset, SQL_STMT_MAX_LEN);
            return SORM_TOO_LONG;
        }
    }

    /* foreign key */
    if((SORM_ENABLE_FOREIGN_KEY & conn->flags) == SORM_ENABLE_FOREIGN_KEY)
    {
	for(i = 0; i < table_desc->columns_num; i ++)
	{
	    column_desc = &(table_desc->columns[i]);
	    if(column_desc->is_foreign_key)
	    {
		ret = snprintf(sql_stmt + offset, SQL_STMT_MAX_LEN - offset + 1,
			" FOREIGN KEY(%s) REFERENCES %s(%s),",
			column_desc->name, column_desc->foreign_table_name, 
			column_desc->foreign_column_name);
		offset += ret;
		if(ret < 0 || offset > SQL_STMT_MAX_LEN)
		{
		    log_debug("snprintf error while constructing sql "
			    "statment, snprintf length(%d) > max length(%d)", 
			    offset, SQL_STMT_MAX_LEN);
		    return SORM_TOO_LONG;
		}
	    }
	}
    }

    sql_stmt[offset - 1] = ')';
    
    for(i = 0; i < table_desc->columns_num; i ++)
    {
	column_desc = &(table_desc->columns[i]);
	if(column_desc->is_foreign_key)
	{
	    ret = snprintf(sql_stmt + offset, SQL_STMT_MAX_LEN - offset + 1,
		    "; CREATE INDEX %s_index ON %s(%s)",
		    column_desc->name, table_desc->name, 
		    column_desc->name);
	    offset += ret;
	    if(ret < 0 || offset > SQL_STMT_MAX_LEN)
	    {
		log_debug("snprintf error while constructing sql "
			"statment, snprintf length(%d) > max length(%d)", 
			offset, SQL_STMT_MAX_LEN);
		return SORM_TOO_LONG;
	    }
	}
    }

    log_debug("prepare stmt : %s", sql_stmt);
    ret = _sqlite3_prepare(conn, sql_stmt, &stmt_handle);

    if(ret != SQLITE_OK)
    {
        log_debug("sqlite3_prepare error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        return SORM_DB_ERROR;
    }

    ret = _sqlite3_step(conn, stmt_handle);

    if(ret != SQLITE_DONE)
    {
        log_debug("sqlite3_step error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        ret_val = SORM_DB_ERROR;
        goto DB_FINALIZE;
    }

    //log_debug("Success return");
    ret_val = SORM_OK;

DB_FINALIZE :
    ret = sqlite3_finalize(stmt_handle);
    if(ret != SQLITE_OK)
    {
        log_debug("sqlite3_finalize error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        return SORM_DB_ERROR;

    }

    return ret_val;
}

int sorm_delete_table(
        const sorm_connection_t *conn, const sorm_table_descriptor_t *table_desc)
{
    char sql_stmt[SQL_STMT_MAX_LEN + 1] = "";
    sqlite3_stmt *stmt_handle = NULL;
    int offset;
    int ret, ret_val;
    int i;
    
    if(conn == NULL)
    {
        log_error("Param conn is NULL.");
        return SORM_ARG_NULL;
    }
    if(table_desc == NULL)
    {
        log_error("Param table_desc is NULL.");
        return SORM_ARG_NULL;
    }

    ret = offset = snprintf(sql_stmt, SQL_STMT_MAX_LEN + 1,
            "DROP TABLE %s", table_desc->name);
    if(ret < 0 || offset > SQL_STMT_MAX_LEN)
    {
        log_debug("snprintf error while constructing sql statment, "
                "snprintf length(%d) > max length(%d)", offset, SQL_STMT_MAX_LEN);
        return SORM_TOO_LONG;
    }

    log_debug("prepare stmt : %s", sql_stmt);
    ret = _sqlite3_prepare(conn, sql_stmt, &stmt_handle);

    if(ret != SQLITE_OK)
    {
        log_debug("sqlite3_prepare error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        return SORM_DB_ERROR;
    }

    ret = _sqlite3_step(conn, stmt_handle);

    if(ret != SQLITE_DONE)
    {
        log_debug("sqlite3_step error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        ret_val = SORM_DB_ERROR;
        goto DB_FINALIZE;
    }

    //log_debug("Success return");
    ret_val = SORM_OK;

DB_FINALIZE :
    ret = sqlite3_finalize(stmt_handle);
    if(ret != SQLITE_OK)
    {
        log_debug("sqlite3_finalize error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        return SORM_DB_ERROR;

    }

    return ret_val;
}

    sorm_table_descriptor_t * 
sorm_new(
        const sorm_table_descriptor_t *init_table_desc)

{
    return sorm_new_array(init_table_desc, 1);
}

    sorm_table_descriptor_t * 
sorm_new_array(
        const sorm_table_descriptor_t *init_table_desc, int n)

{
    /* pointer to connection in new struct */
    sorm_table_descriptor_t *table_desc = NULL;
    sorm_table_descriptor_t *table_desc_pos = NULL;
    int i;

    //log_debug("Start");
    log_debug("Array size(%d)", n);

    if(init_table_desc == NULL)
    {
        log_error("Param init_table_desc is NULL");
        return NULL;
    }

    if(n == 0)
    {
        log_debug("sorm_new_array 0\n");
        return NULL;
    }

    table_desc = mem_malloc(init_table_desc->size * n);
    if(table_desc == NULL)
    {
        log_debug("mem_malloc fail");
        return NULL;
    }
    memset(table_desc, 0, init_table_desc->size * n);

    for(i = 0; i < n; i ++)
    {
        table_desc_pos = (sorm_table_descriptor_t *)((char*)table_desc + 
                init_table_desc->size * (i));
        *table_desc_pos = *init_table_desc;
    }

    //log_debug("Success return");
    return table_desc;
}
void sorm_free(
        sorm_table_descriptor_t *table_desc)
{
    int i;
    //char **txt;

    //log_debug("Start");
    sorm_free_array(table_desc, 1);
    //log_debug("Success return");
}

void sorm_free_array(
        sorm_table_descriptor_t *table_desc, int n)
{
    int i, j;
    sorm_table_descriptor_t *table_desc_pos = NULL;
    //char **txt;

    //log_debug("Start");

    if(table_desc != NULL)
    {
        for(i = 0; i < n; i ++)
        {
            table_desc_pos = (sorm_table_descriptor_t *)((char*)table_desc + 
                    table_desc->size * (i));
            for(j = 0; j < table_desc_pos->columns_num; j ++)
            {
                if(sorm_is_stat_needfree(_get_column_stat(table_desc_pos, j)))
                {
                    //printf("fuck free heap : (%s, %s)\n", table_desc
                    assert((table_desc_pos->columns[j].type == SORM_TYPE_TEXT || 
                                table_desc_pos->columns[j].type == 
                                SORM_TYPE_BLOB));
                    assert(table_desc_pos->columns[j].mem == SORM_MEM_HEAP);
                    mem_free(_heap_member_pointer(table_desc_pos,
                                table_desc_pos->columns[j].offset));
                }
            }
        }
        mem_free(table_desc);
    }
    //log_debug("Success return");
}


int sorm_set_column_value(
        sorm_table_descriptor_t *table_desc, 
        int column_index, const void *value, int value_len)
{
    const sorm_column_descriptor_t *column_desc = NULL;
    char *text = NULL;
    int ret, offset;

    if(table_desc == NULL)
    {
        log_error("Param table_desc is NULL.");
        return SORM_ARG_NULL;
    }
    if(value == NULL)
    {
        log_error("Param value_desc is NULL.");
        return SORM_ARG_NULL;
    }

    column_desc = &table_desc->columns[column_index];
    assert(column_desc != NULL);

    switch(column_desc->type)
    {
        case SORM_TYPE_INT :
            *((int32_t*)((char*)table_desc + column_desc->offset)) = 
                *(int32_t*)value;
            break;
            //TODO TEXT16
        case SORM_TYPE_TEXT :
            if(column_desc->mem == SORM_MEM_HEAP)
            {
                text = (char *)value;
                *(char **)((char*)table_desc + column_desc->offset) = text; 
            }else if(column_desc->mem == SORM_MEM_STACK)
            {
                ret = snprintf((char*)table_desc + column_desc->offset, 
                        column_desc->max_len + 1, "%s", (char *)value);
                offset = ret;
                if(ret < 0 || offset > column_desc->max_len)
                {
                    log_error("set too long text, "
                            "length(%d) > max length(%d)", offset, 
                            column_desc->max_len);
                    return SORM_TOO_LONG;
                }
            }else
            {
                log_error("unknow sorm mem : %d", column_desc->mem);
                return SORM_INVALID_MEM;
            }
            break;
        case SORM_TYPE_DOUBLE :
            *((double*)((char*)table_desc + column_desc->offset)) = 
                *(double*)value;
            break;
        case SORM_TYPE_BLOB :
            if(column_desc->mem == SORM_MEM_HEAP)
            {
                text = value;
                *(char **)((char*)table_desc + column_desc->offset) = text; 
                _set_blob_len(table_desc, column_index, value_len);
            }else if(column_desc->mem == SORM_MEM_STACK)
            {
                if(value_len > column_desc->max_len)
                {
                    log_error("set too long blob, "
                            "length(%d) > max length(%d)", offset, 
                            column_desc->max_len);
                    return SORM_TOO_LONG;
                }
                memcpy((char*)table_desc + column_desc->offset, value, value_len);
                _set_blob_len(table_desc, column_index, value_len);
            }else
            {
                log_error("unknow sorm mem : %d", column_desc->mem);
                return SORM_INVALID_MEM;
            }
            break;
        default :
            log_error("unknow sorm type : %d", column_desc->type);
            return SORM_INVALID_TYPE;
    }

    _add_column_stat(table_desc, column_index, SORM_STAT_VALUED);

    return SORM_OK;
}
/**
 * @brief: insert a new row or update a row in a table
 *
 * @param desc: the descriptor for the table, and the data about the row
 *
 * @return: error code
 */
int sorm_save(
        const sorm_connection_t *conn,
        sorm_table_descriptor_t *table_desc)
{
    char sql_stmt[SQL_STMT_MAX_LEN + 1] = "";
    sqlite3_stmt *stmt_handle = NULL;
    int offset;
    int ret, ret_val;
    int bind_index, i;
    int row_id;
    //log_debug("Start");

    if(table_desc == NULL)
    {
        log_error("Param desc is NULL");
        return SORM_ARG_NULL;
    }
    if(conn == NULL)
    {
        log_error("Param conn is NULL");
        return SORM_ARG_NULL;
    }

    /* sql statment : "UPDATE table_name "*/
    ret = offset = snprintf(sql_stmt, SQL_STMT_MAX_LEN + 1, 
            "REPLACE INTO %s (", table_desc->name);
    if(ret < 0 || offset > SQL_STMT_MAX_LEN)
    {
        log_error("snprintf error while constructing sql statment, "
                "snprintf length(%d) > max length(%d)", offset, SQL_STMT_MAX_LEN);
        return SORM_TOO_LONG;
    }
    
    for(i = 0; i < table_desc->columns_num; i ++)
    {
        if(sorm_is_stat_valued(_get_column_stat(table_desc, i)))
        {
            ret = snprintf(sql_stmt + offset, SQL_STMT_MAX_LEN - offset + 1, 
                    " %s,", table_desc->columns[i].name);
            offset += ret;
            if(ret < 0 || offset > SQL_STMT_MAX_LEN)
            {
                log_error("snprintf error while constructing sql statment, "
                        "snprintf length(%d) > max length(%d)", 
                        offset, SQL_STMT_MAX_LEN);
                return SORM_TOO_LONG;
            }
        }
    }
    sql_stmt[offset - 1] = ')';
    
    ret = snprintf(sql_stmt + offset, SQL_STMT_MAX_LEN - offset + 1, 
            " VALUES (", table_desc->name);
    offset += ret;
    if(ret < 0 || offset > SQL_STMT_MAX_LEN)
    {
        log_error("snprintf error while constructing sql statment, "
                "snprintf length(%d) > max length(%d)", offset, SQL_STMT_MAX_LEN);
        return SORM_TOO_LONG;
    }

    for(i = 0; i < table_desc->columns_num; i ++)
    {
        if(sorm_is_stat_valued(_get_column_stat(table_desc, i)))
        {
            ret = snprintf(sql_stmt + offset, SQL_STMT_MAX_LEN - offset + 1, 
                    " ?,");
            offset += ret;
            if(ret < 0 || offset > SQL_STMT_MAX_LEN)
            {
                log_error("snprintf error while constructing sql statment, "
                        "snprintf length(%d) > max length(%d)", 
                        offset, SQL_STMT_MAX_LEN);
                return SORM_TOO_LONG;
            }
        }
    }
    sql_stmt[offset - 1] = ')';

    log_debug("prepare stmt : %s", sql_stmt);
    
    ret = _sqlite3_prepare(conn, sql_stmt, &stmt_handle);

    if(ret != SQLITE_OK)
    {
        log_error("sqlite3_prepare error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        return SORM_DB_ERROR;
    }

    bind_index = 1;
    for(i = 0; i < table_desc->columns_num; i ++)
    {
        if(sorm_is_stat_valued(_get_column_stat(table_desc, i)))
        {
            ret = _sqlite3_column_bind(
                    conn, stmt_handle, table_desc, i, bind_index);
            if(ret != SORM_OK)
            {
                log_debug("_sqlite3_column_bind error.");
                ret_val = ret;
                goto DB_FINALIZE;
            }
            bind_index ++;
        }
    }

    ret = _sqlite3_step(conn, stmt_handle);

    if(ret != SQLITE_DONE)
    {
        log_debug("sqlite3_step error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        ret_val = SORM_DB_ERROR;
        goto DB_FINALIZE;
    }

    //log_debug("Success return");
    /* get autoset PK value from db */
    if(table_desc->PK_index != SORM_NO_PK)
    {
        /* add lock beacuse  a separte thread can influence the rowid result*/
        if((conn->transaction_num == 0) && 
                (sorm_semaphore_enabled(conn->flags) == 1)) /* no transaction */
        {
            sem_p(conn->sem_key);
        }

        row_id = sqlite3_last_insert_rowid(conn->sqlite3_handle);
        ret = sorm_set_column_value(table_desc, table_desc->PK_index, &row_id, 0);
        if(ret != SORM_OK)
        {
            log_error("set PK value fail.");
            return ret;
        }

        if((conn->transaction_num == 0) && 
                (sorm_semaphore_enabled(conn->flags) == 1)) /* no transaction */
        {
            sem_v(conn->sem_key);
        }
    }

    ret_val = SORM_OK;

DB_FINALIZE :
    ret = sqlite3_finalize(stmt_handle);
    if(ret != SQLITE_OK)
    {
        log_debug("sqlite3_finalize error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        return SORM_DB_ERROR;

    }
    return ret_val;
}
int sorm_delete(
        const sorm_connection_t *conn,
        sorm_table_descriptor_t *table_desc)
{
    char sql_stmt[SQL_STMT_MAX_LEN + 1] = "";
    sqlite3_stmt *stmt_handle = NULL;
    int offset;
    int ret, ret_val;
    const sorm_column_descriptor_t *column_desc = NULL;
    int bind_index, i;

    //log_debug("Start");

    if(table_desc == NULL)
    {
        log_error("Param desc is NULL");
        return SORM_ARG_NULL;
    }

    column_desc = table_desc->columns;

    /* sql statment : "DELETE FROM table_name WHERE"*/
    ret = offset = snprintf(sql_stmt, SQL_STMT_MAX_LEN + 1, 
            "DELETE FROM %s WHERE ", table_desc->name);
    if(ret < 0 || offset > SQL_STMT_MAX_LEN)
    {
        log_debug("snprintf error while constructing sql statment");
        return SORM_TOO_LONG;
    }

    for(i = 0; i < table_desc->columns_num; i ++)
    {
        if(sorm_is_stat_valued(_get_column_stat(table_desc, i)))
        {
            ret = snprintf(sql_stmt + offset, SQL_STMT_MAX_LEN - offset + 1, 
                    " %s=? AND", column_desc[i].name);
            offset += ret;
            if(ret < 0 || offset > SQL_STMT_MAX_LEN)
            {
                log_debug(
                        "snprintf error while constructing sql statment, "
                        "snprintf length(%d) > max length(%d)", 
                        offset, SQL_STMT_MAX_LEN);
                return SORM_TOO_LONG;
            }
        }
    }
    sql_stmt[offset - 4] = '\0';

    log_debug("prepare stmt : %s", sql_stmt);
    ret = _sqlite3_prepare(conn, sql_stmt, &stmt_handle);

    if(ret != SQLITE_OK)
    {
        log_debug("sqlite3_prepare error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        return SORM_DB_ERROR;
    }

    bind_index = 1;
    for(i = 0; i < table_desc->columns_num; i ++)
    {
        if(sorm_is_stat_valued(_get_column_stat(table_desc, i)))
        {
            ret = _sqlite3_column_bind(
                    conn, stmt_handle, table_desc, i, bind_index);
            if(ret != SORM_OK)
            {
                log_debug("_sqlite3_column_bind error");
                ret_val = ret;
                goto DB_FINALIZE;
            }
            bind_index ++;
        }
    }

    ret = _sqlite3_step(conn, stmt_handle);

    if(ret != SQLITE_DONE)
    {
        log_debug("sqlite3_step error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        return SORM_DB_ERROR;
    }

    //log_debug("Success return");
    ret_val = SORM_OK;

DB_FINALIZE :
    ret = sqlite3_finalize(stmt_handle);
    if(ret != SQLITE_OK)
    {
        log_debug("sqlite3_finalize error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        return SORM_DB_ERROR;

    }
    
    //if(ret_val == SORM_OK)
    //{
    //    for(i = 0; i < table_desc->columns_num; i ++)
    //    {
    //        if(_get_column_stat(table_desc, i) != SORM_STAT_NULL)
    //        {
    //            _set_column_stat(table_desc, i, SORM_STAT_VALUED);
    //        }
    //    }
    //}

    return ret_val;
}

int sorm_delete_by_column(
        const sorm_connection_t *conn, const sorm_table_descriptor_t *table_desc,
        int column_index, const void *column_value)
{
    const sorm_column_descriptor_t *column_desc = NULL;

    char filter[SQL_STMT_MAX_LEN + 1];
    int ret;

    //log_debug("Start.");

    if(table_desc == NULL)
    {
        log_debug("Param table_desc is NULL");
        assert(0);
    }
    if(column_value == NULL)
    {
        log_debug("Param column_val is NULL");
        assert(0);
    }

    if(SORM_OK != (ret = _construct_column_filter(table_desc, column_index, 
                    column_value, filter)))
    {
        log_debug("Conster select filter fail.");
        return ret;
    }

    log_debug("Construct filter : %s", filter);

    return sorm_delete_by(conn, table_desc, filter);
}

int sorm_delete_by(
        const sorm_connection_t *conn, const sorm_table_descriptor_t *table_desc, 
        const char *filter)
{
    char sql_stmt[SQL_STMT_MAX_LEN + 1] = "";
    int offset, ret, ret_val;
    sqlite3_stmt *stmt_handle = NULL;

    //log_debug("Start.");

    if(conn == NULL)
    {
        log_debug("Param conn is NULL");
        return SORM_ARG_NULL;
    }
    if(table_desc == NULL)
    {
        log_debug("Param desc is NULL");
        return SORM_ARG_NULL;
    }

    /* construct sqlite3 statement */
    offset = ret = snprintf(sql_stmt, SQL_STMT_MAX_LEN + 1, "DELETE FROM %s ",
            table_desc->name);
    if(ret < 0 || offset > SQL_STMT_MAX_LEN)
    {
        log_debug("snprintf error while constructing sql statment, "
                "snprintf length(%d) > max length(%d)", offset, SQL_STMT_MAX_LEN);
        return SORM_TOO_LONG;
    }
    if(filter != NULL)
    {
        ret = snprintf(sql_stmt + offset, SQL_STMT_MAX_LEN + 1 - offset, 
                "WHERE %s", filter);
        offset += ret;
        if(ret < 0 || offset > SQL_STMT_MAX_LEN)
        {
            log_debug("snprintf error while constructing sql statment, "
                    "snprintf length(%d) > max length(%d)", 
                    offset, SQL_STMT_MAX_LEN);
            return SORM_TOO_LONG;
        }
    }

    log_debug("prepare stmt : %s", sql_stmt);

    /* sqlite3_prepare */
    ret = _sqlite3_prepare(conn, sql_stmt, &stmt_handle);
    if(ret != SQLITE_OK)
    {
        log_debug("sqlite3_prepare error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        return SORM_DB_ERROR;
    }

    ret = _sqlite3_step(conn, stmt_handle);

    if(ret != SQLITE_DONE)
    {
        log_debug("sqlite3_step error(%d) : %s", 
                ret, sqlite3_errmsg(conn->sqlite3_handle));
        ret_val = SORM_DB_ERROR;
        goto DB_FINALIZE;
    }

    //log_debug("Success return");
    ret_val = SORM_OK;

DB_FINALIZE:
    ret = sqlite3_finalize(stmt_handle);
    if(ret != SQLITE_OK)
    {
        log_debug("sqlite3_finalize error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        return SORM_DB_ERROR;
    }

    return ret_val;
}

int sorm_select_one_by_column(
        const sorm_connection_t *conn, const sorm_table_descriptor_t *table_desc,
        const char *columns_name, int column_index, const void *column_value,
        sorm_table_descriptor_t **get_row)
{
    char filter[SQL_STMT_MAX_LEN + 1];
    int ret;
    int n = 1;

    if(column_value == NULL)
    {
        log_debug("Param column_value is NULL");
        return SORM_ARG_NULL;
    }

    ret = _construct_column_filter(table_desc, column_index, column_value, filter);
    if(SORM_OK != ret)
    {
        log_debug("Conster select filter fail.");
        return ret;
    }

    ret = sorm_select_some_array_by(conn, table_desc, columns_name,
            filter, &n, get_row);

    return ret;
}

static int _select(
        select_core_t select_core,
        const sorm_connection_t *conn, const char *columns_name, 
        int tables_num, const sorm_table_descriptor_t **tables_desc,
        int *is_tables_select,
        const char **tables_column_name,
        sorm_join_t join, const char *filter, int *rows_num, 
        void **rows_of_tables) 
{
    assert(tables_num > 0);
    assert(tables_desc != NULL);
    assert(rows_of_tables != NULL);

    char sql_stmt[SQL_STMT_MAX_LEN + 1] = "";
    sqlite3_stmt *stmt_handle = NULL;
    int i;
    int ret_val, offset, ret;
    char *join_str = NULL;
    int max_rows_num;
    select_columns_t *select_columns_of_tables;

    if(conn == NULL)
    {
        log_debug("Param conn is NULL");
        return SORM_ARG_NULL;
    }
    if(columns_name == NULL)
    {
        log_debug("Param columns_name is NULL");
        return SORM_ARG_NULL;
    }
    if(rows_num == NULL)
    {
        log_debug("Param rows_num is NULL");
        return SORM_ARG_NULL;
    }

    select_columns_of_tables = mem_malloc(sizeof(select_columns_t) * tables_num);
    if(select_columns_of_tables == NULL)
    {
        log_error("mem_malloc fail.");
        ret_val = SORM_NOMEM;
        goto RETURN;
    }
    memset(select_columns_of_tables, 0, sizeof(select_columns_t) * tables_num);
    for(i = 0; i < tables_num; i ++)
    {
        select_columns_of_tables[i].indexes_in_result = 
            malloc(sizeof(int) * tables_desc[i]->columns_num);
        if(select_columns_of_tables[i].indexes_in_result == NULL)
        {
            log_error("mem_malloc fail.");
            ret_val = SORM_NOMEM;
            goto RETURN;
        }
        memset(select_columns_of_tables[i].indexes_in_result, 0, 
                sizeof(int) * tables_desc[i]->columns_num);
    }

    max_rows_num = *rows_num; 
    *rows_num = 0;

    ret = _construct_select_stmt(sql_stmt, tables_desc[0]->name,
            columns_name, &offset);
    if(ret != SORM_OK)
    {
        log_debug("_construct_select_stmt error.");
        ret_val = ret;
        goto RETURN;
    }

    if(tables_num > 1)
    {
        switch(join)
        {
            case SORM_INNER_JOIN : join_str = "INNER JOIN"; break;
            case SORM_LEFT_JOIN  : join_str =  "LEFT JOIN"; break;
            default :
                                   log_error("Invalid join type.");
                                   assert(0);
        }
        offset += (ret = snprintf(sql_stmt + offset, 
                    SQL_STMT_MAX_LEN + 1 - offset, 
                    " %s %s ON %s.%s=%s.%s", join_str, tables_desc[1]->name, 
                    tables_desc[0]->name, tables_column_name[0],
                    tables_desc[1]->name, tables_column_name[1]));
        if(ret < 0 || offset > SQL_STMT_MAX_LEN)
        {
            log_debug("snprintf error while constructing filter, "
                    "snprintf length(%d) > max length(%d)", 
                    offset, SQL_STMT_MAX_LEN);
            ret_val = SORM_TOO_LONG;
            goto RETURN;
        }
    }

    ret = _construct_filter_stmt(sql_stmt, filter, &offset);
    if(ret != SORM_OK)
    {
        log_debug("_construct_filter_stmt error.");
        ret_val = ret;
        goto RETURN;
    }

    /* sqlite3_prepare */
    log_debug("prepare stmt : %s", sql_stmt);
    ret = _sqlite3_prepare(conn, sql_stmt, &stmt_handle);
    if(ret != SQLITE_OK)
    {
        log_error("sqlite3_prepare error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        ret_val = SORM_DB_ERROR;
        goto RETURN;
    }

    /* parse columns_name */
    ret = _columns_name_to_select_columns(tables_num, tables_desc, columns_name, 
            select_columns_of_tables);
    if(ret != SORM_OK)
    {
        log_error("_columns_name_to_select_columns fail.");
        ret_val = ret;
        goto DB_FINALIZE;
    }

    ret_val = select_core(conn, stmt_handle, tables_num, tables_desc,
            is_tables_select, select_columns_of_tables,
            max_rows_num, rows_num,  rows_of_tables);

DB_FINALIZE:

    ret = sqlite3_finalize(stmt_handle);
    if(ret != SQLITE_OK)
    {
        log_debug("sqlite3_finalize error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        ret_val = SORM_DB_ERROR;
    }

RETURN :

    if(select_columns_of_tables != NULL)
    {
        for(i = 0; i < tables_num; i ++)
        {
            if(select_columns_of_tables[i].indexes_in_result != NULL) 
            {
                mem_free(select_columns_of_tables[i].indexes_in_result);
            }
        }
        mem_free(select_columns_of_tables);
    }

    log_debug("select row number(%d)", *rows_num);

    if((ret_val == SORM_OK) && ((*rows_num) == 0))
    {
        return SORM_NOEXIST;
    }

    return ret_val;
}

int _select_some_array_core(
        const sorm_connection_t *conn, sqlite3_stmt* stmt_handle,
        int tables_num, const sorm_table_descriptor_t **tables_desc,
        const int *is_table_select, 
        const select_columns_t *select_columns_of_tables, 
        int max_rows_num, int *rows_num, void **rows_of_tables)
{
    assert(conn != NULL);
    assert(stmt_handle != NULL);
    assert(tables_desc != NULL);
    assert(select_columns_of_tables != NULL);
    assert(rows_num != NULL);
    assert(rows_of_tables != NULL);

    int step_ret, ret;
    int ret_val;
    int i;

    memset(rows_of_tables, 0, sizeof(void*) * tables_num);
    
    /* if max_rows_num, the sorm new_array will return NULL, then the function 
     * will return SORM_NOMEM, but in this case ,the function need to return 
     * SORM_OK, so we return here*/
    if(max_rows_num == 0)       
    {
        log_debug("max_row_num is 0");
        *rows_num = 0;
        return SORM_OK;
    }
    
    for(i = 0; i < tables_num; i ++)
    {
        if((select_columns_of_tables[i].columns_num != 0) && 
                (is_table_select[i] == 1))
        {
            rows_of_tables[i] = sorm_new_array(tables_desc[i], max_rows_num);
            if(rows_of_tables[i] == NULL)
            {
                log_error("new sorm error.");
                ret_val = SORM_NOMEM;
                goto RETURN;
            }
        }
    }

    step_ret = SQLITE_DONE; //init to SQLITE_DONE, for row_numbe can ben zero
    while((*rows_num) < max_rows_num)
    {
        step_ret = _sqlite3_step(conn, stmt_handle);
        if(step_ret == SQLITE_ROW)
        {
            //a new row
            /* get data of the row for table1*/
            for(i = 0; i < tables_num; i ++)
            {
                if((select_columns_of_tables[i].columns_num != 0) &&
                        (is_table_select[i] == 1))
                {
                    ret = _parse_select_result(
                            conn, stmt_handle, 
                            select_columns_of_tables[i].indexes_in_result, 
                            (sorm_table_descriptor_t*)((char*)rows_of_tables[i] +
                                tables_desc[i]->size * (*rows_num)));
                    if(ret != SORM_OK)
                    {
                        ret_val = ret;
                        log_debug("_parse_select_result error."); 
                        goto RETURN;
                    }
                }
            }
            (*rows_num)++;
            log_debug("now get : %d", *rows_num);
        }else
        {
            break;
        }
    }

    if((step_ret != SQLITE_DONE) && (step_ret != SQLITE_ROW))
    {
        log_error("sqlite3_step error(%d) : %s", 
                step_ret, sqlite3_errmsg(conn->sqlite3_handle));
        ret_val = SORM_DB_ERROR;
        goto RETURN;
    }

    ret_val = SORM_OK;

RETURN :

    if((ret_val != SORM_OK) || (*rows_num == 0))
    {
        for(i = 0; i < tables_num; i ++)
        {
            mem_free(rows_of_tables[i]);
            rows_of_tables[i] = NULL;
        }
    }

    return ret_val;
}

int _select_some_list_core(
        const sorm_connection_t *conn, sqlite3_stmt* stmt_handle,
        int tables_num, const sorm_table_descriptor_t **tables_desc,
        const int *is_table_select, 
        const select_columns_t *select_columns_of_tables, 
        int max_rows_num, int *rows_num, void **rows_of_tables)
{
    assert(conn != NULL);
    assert(stmt_handle != NULL);
    assert(tables_desc != NULL);
    assert(select_columns_of_tables != NULL);
    assert(rows_num != NULL);
    assert(rows_of_tables != NULL);

    int step_ret, ret;
    sorm_list_t *list_entry;
    int ret_val;
    int i;

    if(max_rows_num == 0)
    {
        log_debug("max_row_num is 0");
        *rows_num = 0;
        return SORM_OK;
    }

    memset(rows_of_tables, 0, sizeof(void*) * tables_num);
    for(i = 0; i < tables_num; i ++)
    {
        if((select_columns_of_tables[i].columns_num != 0) &&
                (is_table_select[i] == 1))
        {
            rows_of_tables[i] = mem_malloc(sizeof(sorm_list_t));
            if(rows_of_tables[i] == NULL)
            {
                log_error("malloc for sorm_list error.");
                ret_val = SORM_NOMEM;
                goto RETURN;
            }
            INIT_LIST_HEAD((sorm_list_t *)rows_of_tables[i]);
        }
    }

    step_ret = SQLITE_DONE; //init to SQLITE_DONE, for row_numbe can ben zero
    while((*rows_num) < max_rows_num)
    {
        step_ret = _sqlite3_step(conn, stmt_handle);
        if(step_ret == SQLITE_ROW)
        {
            //a new row
            /* get data of the row for table1*/
            for(i = 0; i < tables_num; i ++)
            {
                if((select_columns_of_tables[i].columns_num != 0) &&
                        (is_table_select[i] == 1))
                {
                    list_entry = mem_malloc(sizeof(sorm_list_t));
                    if(list_entry == NULL)
                    {
                        log_error("mem_malloc for list_entry fail.");
                        ret_val = SORM_NOMEM;
                        goto RETURN;
                    }
                    list_add_tail(list_entry, rows_of_tables[i]);
                    list_entry->data = sorm_new(tables_desc[i]);
                    if(list_entry->data == NULL)
                    {
                        log_debug("New sorm error");
                        ret_val =  SORM_NOMEM;
                        goto RETURN;
                    }
                    ret = _parse_select_result(
                            conn, stmt_handle, 
                            select_columns_of_tables[i].indexes_in_result, 
                            (sorm_table_descriptor_t*)list_entry->data);
                    if(ret != SORM_OK)
                    {
                        ret_val = ret;
                        log_debug("_parse_select_result error."); 
                        goto RETURN;
                    }
                }
            }

            (*rows_num)++;
            log_debug("now get : %d", *rows_num);
        }else
        {
            break;
        }
    }

    if((step_ret != SQLITE_DONE) && (step_ret != SQLITE_ROW))
    {
        log_error("sqlite3_step error(%d) : %s", 
                step_ret, sqlite3_errmsg(conn->sqlite3_handle));
        ret_val = SORM_DB_ERROR;
        goto RETURN;
    }

    ret_val = SORM_OK;

RETURN :

    if((ret_val != SORM_OK) || (*rows_num == 0))
    {
        for(i = 0; i < tables_num; i ++)
        {
            sorm_list_free(rows_of_tables[i]);
            rows_of_tables[i] = NULL;
        }
    }

    return ret_val;
}

int _select_all_list_core(
        const sorm_connection_t *conn, sqlite3_stmt* stmt_handle,
        int tables_num, const sorm_table_descriptor_t **tables_desc,
        const int *is_table_select, 
        const select_columns_t *select_columns_of_tables, 
        int max_rows_num, int *rows_num, void **rows_of_tables)
{
    assert(conn != NULL);
    assert(stmt_handle != NULL);
    assert(tables_desc != NULL);
    assert(select_columns_of_tables != NULL);
    assert(rows_num != NULL);
    assert(rows_of_tables != NULL);

    int ret;
    sorm_list_t *list_entry;
    int ret_val;
    int i;

    memset(rows_of_tables, 0, sizeof(void*) * tables_num);
    for(i = 0; i < tables_num; i ++)
    {
        if((select_columns_of_tables[i].columns_num != 0) &&
                (is_table_select[i] == 1))
        {
            rows_of_tables[i] = mem_malloc(sizeof(sorm_list_t));
            if(rows_of_tables[i] == NULL)
            {
                log_error("malloc for sorm_list error.");
                ret_val = SORM_NOMEM;
                goto RETURN;
            }
            INIT_LIST_HEAD((sorm_list_t *)rows_of_tables[i]);
        }
    }

    while(1)
    {
        ret = _sqlite3_step(conn, stmt_handle);
        if(ret == SQLITE_ROW)
        {
            //a new row
            /* get data of the row for table1*/
            for(i = 0; i < tables_num; i ++)
            {
                if((select_columns_of_tables[i].columns_num != 0) &&
                        (is_table_select[i] == 1))
                {
                    list_entry = mem_malloc(sizeof(sorm_list_t));
                    if(list_entry == NULL)
                    {
                        log_error("mem_malloc for list_entry fail.");
                        ret_val = SORM_NOMEM;
                        goto RETURN;
                    }
                    list_add_tail(list_entry, rows_of_tables[i]);
                    list_entry->data = sorm_new(tables_desc[i]);
                    if(list_entry->data == NULL)
                    {
                        log_debug("New sorm error");
                        ret_val =  SORM_NOMEM;
                        goto RETURN;
                    }
                    ret = _parse_select_result(
                            conn, stmt_handle, 
                            select_columns_of_tables[i].indexes_in_result, 
                            (sorm_table_descriptor_t*)list_entry->data);
                    if(ret != SORM_OK)
                    {
                        ret_val = ret;
                        log_debug("_parse_select_result error."); 
                        goto RETURN;
                    }
                }
            }

            (*rows_num)++;
            log_debug("now get : %d", *rows_num);
        }else
        {
            break;
        }
    }

    if(ret != SQLITE_DONE)
    {
        log_error("sqlite3_step error(%d) : %s", 
                ret, sqlite3_errmsg(conn->sqlite3_handle));
        ret_val = SORM_DB_ERROR;
        goto RETURN;
    }

    ret_val = SORM_OK;

RETURN :

    if((ret_val != SORM_OK) || (*rows_num == 0))
    {
        for(i = 0; i < tables_num; i ++)
        {
            sorm_list_free(rows_of_tables[i]);
            rows_of_tables[i] = NULL;
        }
    }

    return ret_val;
}

int sorm_select_some_array_by(
        const sorm_connection_t *conn, const sorm_table_descriptor_t *table_desc,
        const char *columns_name, const char *filter, 
        int *rows_num, sorm_table_descriptor_t **rows_p)
{
    int ret;
    sorm_table_descriptor_t *rows = NULL;
    int is_tables_select;

    //log_debug("Start.");
    is_tables_select = (rows_p == NULL) ? 0 : 1;
    ret = _select(_select_some_array_core,
            conn, columns_name, 1, &table_desc, 
            &is_tables_select, NULL, 0, filter, 
            rows_num, (void **)&rows);

    if(rows_p != NULL)
    {
        (*rows_p) = rows;
    }

    return ret;
}


int sorm_select_some_list_by(
        const sorm_connection_t *conn, const sorm_table_descriptor_t *table_desc,
        const char *columns_name, const char *filter, 
        int *rows_num, sorm_list_t **rows_head_p)
{
    int ret;
    sorm_list_t *rows_head = NULL;
    int is_table_select;

    is_table_select = (rows_head_p == NULL) ? 0 : 1;

    ret = _select(_select_some_list_core,
            conn, columns_name, 1, &table_desc, 
            &is_table_select, NULL, 0, filter, 
            rows_num, (void **)&rows_head);

    if(rows_head_p != NULL)
    {
        (*rows_head_p) = rows_head;
    }

    return ret;
}

int sorm_select_all_array_by(
        const sorm_connection_t *conn, const sorm_table_descriptor_t *table_desc,
        const char *columns_name, const char *filter, 
        int *n, sorm_table_descriptor_t **get_row)
{
    int ret;
    sorm_list_t *row_head = NULL, **row_head_p = NULL;
    sorm_table_descriptor_t *_get_row = NULL;

    //log_debug("Start.");

    if(get_row != NULL)
    {
        *get_row = NULL;
        row_head_p = &row_head;
    }

    ret = sorm_select_all_list_by(conn, table_desc, columns_name, filter, n, 
            row_head_p);

    if(ret != SORM_OK)
    {
        return ret;
    }
    if((*n) == 0)
    {
        log_debug("NOEXIST");
        return SORM_NOEXIST;
    }

    if(get_row != NULL)
    {
        _get_row = sorm_new_array(table_desc, *n);
        if(_get_row == NULL)
        {
            log_debug("New sorm array error");
            sorm_list_free(row_head);
            return SORM_NOMEM;
        }
        _list_cpy_free(table_desc, row_head, *n, _get_row, mem_free);
        *get_row = _get_row;
    }

    //log_debug("Success return");
    return SORM_OK;
}

int sorm_select_all_list_by(
        const sorm_connection_t *conn, const sorm_table_descriptor_t *table_desc,
        const char *columns_name, const char *filter, 
        int *rows_num, sorm_list_t **rows_head_p)
{
    int ret;
    sorm_list_t *rows_head = NULL;
    int is_table_select;

    is_table_select = (rows_head_p == NULL) ? 0 : 1;

    ret = _select(_select_all_list_core,
            conn, columns_name, 1, &table_desc, 
            &is_table_select, NULL, 0, filter, 
            rows_num, (void **)&rows_head);

    if(rows_head_p != NULL)
    {
        (*rows_head_p) = rows_head;
    }

    return ret;
}

int sorm_select_some_array_by_join(
        const sorm_connection_t *conn, const char *columns_name,
        const sorm_table_descriptor_t *table1_desc, 
        const char *table1_column_name,
        const sorm_table_descriptor_t *table2_desc, 
        const char *table2_column_name,
        sorm_join_t join, const char *filter, int *rows_num,
        sorm_table_descriptor_t **table1_rows, 
        sorm_table_descriptor_t **table2_rows)
{
    int ret;
    sorm_table_descriptor_t *rows[2] = { NULL, NULL };
    const sorm_table_descriptor_t *tables_desc[2];
    const char *tables_column_name[2];
    int is_tables_select[2];

    tables_desc[0] = table1_desc;
    tables_desc[1] = table2_desc;

    tables_column_name[0] = table1_column_name;
    tables_column_name[1] = table2_column_name;

    is_tables_select[0] = (table1_rows == NULL) ? 0 : 1;
    is_tables_select[1] = (table2_rows == NULL) ? 0 : 1;

    ret = _select(_select_some_array_core, 
            conn, columns_name, 2, tables_desc, is_tables_select,
            tables_column_name, join,
            filter, rows_num, (void **)rows);

    if(table1_rows != NULL)
    {
        (*table1_rows) = rows[0];
    }
    if(table2_rows != NULL)
    {
        (*table2_rows) = rows[1];
    }

    return ret;
}

int sorm_select_some_list_by_join(
        const sorm_connection_t *conn, const char *columns_name,
        const sorm_table_descriptor_t *table1_desc, 
        const char *table1_column_name,
        const sorm_table_descriptor_t *table2_desc, const 
        char *table2_column_name,
        sorm_join_t join, const char *filter, int *rows_num,
        sorm_list_t **table1_rows_head, 
        sorm_list_t **table2_rows_head)
{
    int ret;
    sorm_list_t *rows_head[2] = { NULL, NULL};
    const sorm_table_descriptor_t *tables_desc[2];
    const char *tables_column_name[2];
    int is_tables_select[2];

    tables_desc[0] = table1_desc;
    tables_desc[1] = table2_desc;

    tables_column_name[0] = table1_column_name;
    tables_column_name[1] = table2_column_name;
    
    is_tables_select[0] = (table1_rows_head == NULL) ? 0 : 1;
    is_tables_select[1] = (table2_rows_head == NULL) ? 0 : 1;

    ret = _select(_select_some_list_core, 
            conn, columns_name, 2, tables_desc, is_tables_select, 
            tables_column_name, join,
            filter, rows_num, (void **)rows_head);

    if(table1_rows_head != NULL)
    {
        (*table1_rows_head) = rows_head[0];
    }
    if(table2_rows_head != NULL)
    {
        (*table2_rows_head) = rows_head[1];
    }

    return ret;
}

int sorm_select_all_array_by_join(
        const sorm_connection_t *conn, const char *columns_name,
        const sorm_table_descriptor_t *table1_desc, 
        const char *table1_column_name,
        const sorm_table_descriptor_t *table2_desc, 
        const char *table2_column_name,
        sorm_join_t join, const char *filter, int *rows_num,
        sorm_table_descriptor_t **table1_rows, 
        sorm_table_descriptor_t **table2_rows)
{
    int ret;
    sorm_list_t *table1_row_head = NULL, *table2_row_head = NULL,
                **table1_row_head_p = NULL, **table2_row_head_p = NULL;
    sorm_table_descriptor_t *_table1_rows = NULL, *_table2_rows = NULL;

    if(table1_rows != NULL)
    {
        *table1_rows = NULL;
        table1_row_head_p = &table1_row_head;
    }
    if(table2_rows != NULL)
    {
        *table2_rows = NULL;
        table2_row_head_p = &table2_row_head;
    }

    ret = sorm_select_all_list_by_join(
            conn, columns_name, table1_desc, table1_column_name,
            table2_desc, table2_column_name, join, filter, rows_num,
            table1_row_head_p, table2_row_head_p);

    if(ret != SORM_OK)
    {
        return ret;
    }

    log_debug("get row number : %d\n", (*rows_num));

    if((table1_rows != NULL) && (table1_row_head != NULL))
    {
        _table1_rows = sorm_new_array(table1_desc, *rows_num);
        if(_table1_rows == NULL)
        {
            log_debug("New sorm array error");
            return SORM_NOMEM;
        }
        _list_cpy_free(table1_desc, table1_row_head, *rows_num, 
                _table1_rows, mem_free);
    
        *table1_rows = _table1_rows;
    }
    if((table2_rows != NULL) && (table2_row_head != NULL))
    {
        _table2_rows = sorm_new_array(table2_desc, *rows_num);
        if(_table2_rows == NULL)
        {
            log_debug("New sorm array error");
            return SORM_NOMEM;
        }
        _list_cpy_free(table2_desc, table2_row_head, *rows_num,
                _table2_rows, mem_free);
        *table2_rows = _table2_rows;
    }

    return SORM_OK;
}

int sorm_select_all_list_by_join(
        const sorm_connection_t *conn,const char *columns_name, 
        const sorm_table_descriptor_t *table1_desc, 
        const char *table1_column_name,
        const sorm_table_descriptor_t *table2_desc, 
        const char *table2_column_name,
        sorm_join_t join, const char *filter, int *rows_num,
        sorm_list_t **table1_rows_head, sorm_list_t **table2_rows_head)
{
    int ret;
    sorm_list_t *rows_head[2] = { NULL, NULL};
    const sorm_table_descriptor_t *tables_desc[2];
    const char *tables_column_name[2];
    int is_tables_select[2];

    tables_desc[0] = table1_desc;
    tables_desc[1] = table2_desc;

    tables_column_name[0] = table1_column_name;
    tables_column_name[1] = table2_column_name;
    
    is_tables_select[0] = (table1_rows_head == NULL) ? 0 : 1;
    is_tables_select[1] = (table2_rows_head == NULL) ? 0 : 1;

    ret = _select(_select_all_list_core, 
            conn, columns_name, 2, tables_desc, is_tables_select, 
            tables_column_name, join,
            filter, rows_num, (void **)rows_head);

    if(table1_rows_head != NULL)
    {
        (*table1_rows_head) = rows_head[0];
    }
    if(table2_rows_head != NULL)
    {
        (*table2_rows_head) = rows_head[1];
    }

    return ret;
}


int sorm_begin_transaction(sorm_connection_t *conn)
{
    sqlite3_stmt *stmt_handle = NULL;
    char sql_stmt[SQL_STMT_MAX_LEN + 1] = "";
    int offset;
    int ret, ret_val;

    if(conn == NULL)
    {
        log_debug("Param conn is NULL");
        return SORM_ARG_NULL;
    }
    //log_debug("Start");

    if(conn->transaction_num == 0) /* first transaction */
    {
	if(sorm_semaphore_enabled(conn->flags) == 1)
	{
	    sem_p(conn->sem_key);
	}
        if(conn->begin_trans_stmt == NULL)
        {
            ret = offset = snprintf(sql_stmt, SQL_STMT_MAX_LEN + 1, 
                    "BEGIN TRANSACTION");
            if(ret < 0 || offset > SQL_STMT_MAX_LEN)
            {
                log_debug("snprintf error while constructing sql "
                        "statment, snprintf length(%d) > max length(%d)", 
                        offset, SQL_STMT_MAX_LEN);
                return SORM_TOO_LONG;
            }

            log_debug("prepare stmt : %s", sql_stmt);
            ret = sqlite3_prepare(conn->sqlite3_handle, sql_stmt, 
                    SQL_STMT_MAX_LEN, &stmt_handle, NULL);

            if(ret != SQLITE_OK)
            {
                log_debug("sqlite3_prepare error : %s", 
                        sqlite3_errmsg(conn->sqlite3_handle));
                return SORM_DB_ERROR;
            }

            conn->begin_trans_stmt = stmt_handle;
        }else
        {
            stmt_handle = conn->begin_trans_stmt;
        }

        ret = sqlite3_step(stmt_handle);

        if(ret != SQLITE_DONE)
        {
            log_debug("sqlite3_step error : %s", 
                    sqlite3_errmsg(conn->sqlite3_handle));
            ret_val = SORM_DB_ERROR;
        }

        ret_val = SORM_OK;

DB_FINALIZE :
        if(ret_val == SORM_OK)
        {
            conn->transaction_num ++; //only ++ when success return
            //log_debug("Success return");
        }
        return ret_val;
    }else
    {
        conn->transaction_num ++;
        //log_debug("Success return");
        return SORM_OK;
    }
}

int sorm_commit_transaction(sorm_connection_t *conn)
{
    sqlite3_stmt *stmt_handle = NULL;
    char sql_stmt[SQL_STMT_MAX_LEN + 1] = "";
    int offset;
    int ret, ret_val;

    //log_debug("Start");

    if(conn == NULL)
    {
        log_debug("Param conn is NULL");
        return SORM_ARG_NULL;
    }

    if(conn->transaction_num == 0) /* commit transaction without begin before*/
    {
        log_debug("Commit transaction without begin before.");
        return SORM_OK;
    }else if(conn->transaction_num == 1) /* last commit */
    {
        if(conn->commit_trans_stmt == NULL)
        {
            ret = offset = snprintf(sql_stmt, SQL_STMT_MAX_LEN + 1, 
                    "COMMIT TRANSACTION");
            if(ret < 0 || offset > SQL_STMT_MAX_LEN)
            {
                log_debug("snprintf error while constructing sql " 
                        "statment, snprintf length(%d) > max length(%d)", 
                        offset, SQL_STMT_MAX_LEN);
                return SORM_TOO_LONG;
            }
            log_debug("prepare stmt : %s", sql_stmt);
            ret = sqlite3_prepare(conn->sqlite3_handle, sql_stmt, 
                    SQL_STMT_MAX_LEN, &stmt_handle, NULL);

            if(ret != SQLITE_OK)
            {
                log_debug("sqlite3_prepare error : %s", 
                        sqlite3_errmsg(conn->sqlite3_handle));
                return SORM_DB_ERROR;
            }

            conn->commit_trans_stmt = stmt_handle;
        }else
        {
            stmt_handle = conn->commit_trans_stmt;
        }

        ret = sqlite3_step(stmt_handle);
	if(sorm_semaphore_enabled(conn->flags) == 1)
	{
	    sem_v(conn->sem_key);
	}

        if(ret != SQLITE_DONE)
        {
            log_debug("sqlite3_step error : %s", 
                    sqlite3_errmsg(conn->sqlite3_handle));
            ret_val = SORM_DB_ERROR;
        }

        ret_val = SORM_OK;

DB_FINALIZE :
        if(ret_val == SORM_OK)
        {
            conn->transaction_num --; //only ++ when success return
            //log_debug("Success return");
        }
        return ret_val;
    }else
    {
        conn->transaction_num --;
        //log_debug("Success return");
        return SORM_OK;
    }
}

int sorm_rollback_transaction(sorm_connection_t *conn)
{
    sqlite3_stmt *stmt_handle = NULL;
    char sql_stmt[SQL_STMT_MAX_LEN + 1] = "";
    int offset;
    int ret, ret_val;

    //log_debug("Start");

    if(conn == NULL)
    {
        log_debug("Param conn is NULL");
        return SORM_ARG_NULL;
    }

    if(conn->transaction_num == 0) /* commit transaction without begin before*/
    {
        log_debug("Rollback transaction without begin before.");
        return SORM_OK;
    }else if(conn->transaction_num == 1) /* last rollback */
    {
        ret = offset = snprintf(sql_stmt, SQL_STMT_MAX_LEN + 1, 
                "ROLLBACK TRANSACTION");
        if(ret < 0 || offset > SQL_STMT_MAX_LEN)
        {
            log_debug("snprintf error while constructing sql " 
                    "statment, snprintf length(%d) > max length(%d)", 
                    offset, SQL_STMT_MAX_LEN);
            return SORM_TOO_LONG;
        }
        log_debug("prepare stmt : %s", sql_stmt);
        ret = sqlite3_prepare(conn->sqlite3_handle, sql_stmt, 
                SQL_STMT_MAX_LEN, &stmt_handle, NULL);

        if(ret != SQLITE_OK)
        {
            log_debug("sqlite3_prepare error : %s", 
                    sqlite3_errmsg(conn->sqlite3_handle));
            return SORM_DB_ERROR;
        }

        ret = sqlite3_step(stmt_handle);
	if(sorm_semaphore_enabled(conn->flags) == 1)
	{
	    sem_v(conn->sem_key);
	}

        if(ret != SQLITE_DONE)
        {
            log_debug("sqlite3_step error : %s", 
                    sqlite3_errmsg(conn->sqlite3_handle));
            ret_val = SORM_DB_ERROR;
        }
    
        ret_val = SORM_OK;

DB_FINALIZE :
        ret = sqlite3_finalize(stmt_handle);
        if(ret != SQLITE_OK)
        {
            log_debug("sqlite3_finalize error : %s", 
                    sqlite3_errmsg(conn->sqlite3_handle));
            return SORM_DB_ERROR;

        }

        if(ret_val == SORM_OK)
        {
            conn->transaction_num --; //only -- when success return
            //log_debug("Success return");
        }
        return ret_val;
    }else
    {
        conn->transaction_num --;
        //log_debug("Success return");
        return SORM_OK;
    }
}
//int sorm_select_callback_by(
//        const sorm_connection_t *conn, const sorm_table_descriptor_t *desc,
//        const char *filter,
//        (void*)(*callback)(void*))
//{
//    const sorm_column_descriptor_t *column_desc = NULL;
//
//    if(conn == NULL)
//    {
//        log_debug("Param conn is NULL");
//        return SORM_INVALID_NUM;
//    }
//    if(table_desc == NULL)
//    {
//        log_debug("Param desc is NULL");
//        return SORM_INVALID_NUM;
//    }
//    
//    column_desc = table_desc->columns;
//    
//    /* construct sqlite3 statement */
//    ret = _construct_select_stmt(sql_stmt, table_desc->name, 
//            columns_name, &offset);
//    if(ret != SORM_OK)
//    {
//        log_debug("_construct_select_stmt error.");
//        return ret;
//    }
//    ret = _construct_filter_stmt(sql_stmt, filter, &offset);
//    if(ret != SORM_OK)
//    {
//        log_debug("_construct_filter_stmt error.");
//        return ret;
//    }
//
//    log_debug("prepare stmt : %s", sql_stmt);
//
//    /* sqlite3_prepare */
//    ret = sqlite3_prepare(conn->sqlite3_handle, sql_stmt, 
//            SQL_STMT_MAX_LEN, &stmt_handle, NULL);
//    if(ret != SQLITE_OK)
//    {
//        log_debug("sqlite3_prepare error : %s", 
//                sqlite3_errmsg(conn->sqlite3_handle));
//        return SORM_DB_ERROR;
//    }
//    
//    while(1)
//    {
//        ret = SQLITE_BUSY;
//        while(ret == SQLITE_BUSY)
//        {
//            ret = sqlite3_step(stmt_handle);
//        }
//        if(ret == SQLITE_ROW) //a new row
//        {
//            (*n)++;
//            log_debug("now get row number(%d)", *n);
//            if(get_row_head != NULL)
//            {
//                ret = _parse_select_result(
//                        conn, stmt_handle, table_desc, 
//                        select_columns_of_table.indexes_in_result, 
//                        _row_head);
//                if(ret != SORM_OK)
//                {
//                    ret_val = ret;
//                    goto DB_FINALIZE;
//                }
//            }
//        }else	//all rows have been selected
//        {
//            break;
//        }
//    }
//    
//}

int sorm_close(sorm_connection_t *conn)
{
    int ret, i;

    if(conn == NULL)
    {
        log_debug("Param conn is NULL");
        return SORM_ARG_NULL;
    }

    //log_debug("Start");
    //finalize pre-prepared stmt
    if(conn->begin_trans_stmt != NULL)
    {
        ret = sqlite3_finalize(conn->begin_trans_stmt);
        if(ret != SQLITE_OK)
        {
            log_debug("sqlite3_finalize error : %s", 
                    sqlite3_errmsg(conn->sqlite3_handle));
            return SORM_DB_ERROR;
        }
        conn->begin_trans_stmt = NULL;
    }
    if(conn->commit_trans_stmt != NULL)
    {
        log_debug("commit_trans_stmt finalize.");
        ret = sqlite3_finalize(conn->commit_trans_stmt);
        if(ret != SQLITE_OK)
        {
            log_debug("sqlite3_finalize error : %s", 
                    sqlite3_errmsg(conn->sqlite3_handle));
            return SORM_DB_ERROR;
        }
        conn->commit_trans_stmt = NULL;
    }

    ret = SQLITE_BUSY;

    while(ret == SQLITE_BUSY)
    {
        ret = sqlite3_close(conn->sqlite3_handle);
    }

    if(ret != SQLITE_OK)
    {
        log_debug("sqlite3_close error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        return SORM_DB_ERROR;
    }

    mem_free(conn->db_file_path);
    mem_free(conn);
    
    //log_debug("Success return");
    return SORM_OK;
}

static inline void _construct_index_suffix(
	char *index_suffix, char *columns_name)
{
    assert(index_suffix != NULL);
    assert(columns_name != NULL);
    
    char *index_suffix_iter, *columns_name_iter;
    
    index_suffix_iter = index_suffix;
    columns_name_iter = columns_name;
    
    while((*columns_name_iter) != '\0')
    {
	if((*columns_name_iter) == COLUMN_NAMES_DELIMITER)
	{
	    (*index_suffix_iter) = '_';
	    index_suffix_iter ++;
	}else if((*columns_name_iter) != ' ')
	{
	    (*index_suffix_iter) = (*columns_name_iter);
	    index_suffix_iter ++;
	}
	columns_name_iter ++;
    }
    (*index_suffix_iter) = '\0';
}

int sorm_create_index(
	const sorm_connection_t *conn, 
	sorm_table_descriptor_t *table_desc, char *columns_name)
{
    char sql_stmt[SQL_STMT_MAX_LEN + 1] = "";
    sqlite3_stmt *stmt_handle = NULL;
    int offset, columns_name_len;
    int ret, ret_val, i;
    char *index_suffix;
    
    if(conn == NULL)
    {
        log_error("Param conn is NULL.");
        return SORM_ARG_NULL;
    }
    if(table_desc == NULL)
    {
        log_error("Param table_desc is NULL.");
        return SORM_ARG_NULL;
    }
    if(columns_name == NULL)
    {
        log_error("Param columns_name is NULL.");
        return SORM_ARG_NULL;
    }

    /* construct the suffix for index name */
    columns_name_len = strlen(columns_name);
    index_suffix = mem_malloc(columns_name_len + 1); /* +1 for \0 */
    _construct_index_suffix(index_suffix, columns_name);

    ret = offset = snprintf(sql_stmt, SQL_STMT_MAX_LEN + 1,
	    "CREATE INDEX index_%s ON %s(%s)", 
	    index_suffix, table_desc->name, columns_name);

    mem_free(index_suffix);
    
    if(ret < 0 || offset > SQL_STMT_MAX_LEN)
    {
        log_error("snprintf error while constructing sql statment, "
                "snprintf length(%d) > max length(%d)", offset, SQL_STMT_MAX_LEN);
        return SORM_TOO_LONG;
    }
    
    log_debug("prepare stmt : %s", sql_stmt);
    
    ret = _sqlite3_prepare(conn, sql_stmt, &stmt_handle);

    if(ret != SQLITE_OK)
    {
        log_error("sqlite3_prepare error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        return SORM_DB_ERROR;
    }
    
    ret = _sqlite3_step(conn, stmt_handle);

    if(ret != SQLITE_DONE)
    {
        log_debug("sqlite3_step error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        ret_val = SORM_DB_ERROR;
        goto DB_FINALIZE;
    }

    //log_debug("Success return");
    ret_val = SORM_OK;

DB_FINALIZE :

    ret = sqlite3_finalize(stmt_handle);
    if(ret != SQLITE_OK)
    {
        log_debug("sqlite3_finalize error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        return SORM_DB_ERROR;

    }
    return ret_val;
}

int sorm_drop_index(
	const sorm_connection_t *conn,
	char *columns_name)
{
    char sql_stmt[SQL_STMT_MAX_LEN + 1] = "";
    sqlite3_stmt *stmt_handle = NULL;
    int offset, columns_name_len;
    int ret, ret_val, i;
    char *index_suffix;

    if(conn == NULL)
    {
	log_error("Param conn is NULL.");
	return SORM_ARG_NULL;
    }
    if(columns_name == NULL)
    {
	log_error("Param columns_name is NULL.");
	return SORM_ARG_NULL;
    }

    /* construct the suffix for index name */
    columns_name_len = strlen(columns_name);
    index_suffix = mem_malloc(columns_name_len + 1);
    _construct_index_suffix(index_suffix, columns_name);

    ret = offset = snprintf(sql_stmt, SQL_STMT_MAX_LEN + 1,
	    "DROP INDEX index_%s", index_suffix);

    mem_free(index_suffix);
    
    if(ret < 0 || offset > SQL_STMT_MAX_LEN)
    {
        log_error("snprintf error while constructing sql statment, "
                "snprintf length(%d) > max length(%d)", offset, SQL_STMT_MAX_LEN);
        return SORM_TOO_LONG;
    }
    
    log_debug("prepare stmt : %s", sql_stmt);
    
    ret = _sqlite3_prepare(conn, sql_stmt, &stmt_handle);

    if(ret != SQLITE_OK)
    {
        log_error("sqlite3_prepare error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        return SORM_DB_ERROR;
    }
    
    ret = _sqlite3_step(conn, stmt_handle);

    if(ret != SQLITE_DONE)
    {
        log_debug("sqlite3_step error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        ret_val = SORM_DB_ERROR;
        goto DB_FINALIZE;
    }

    //log_debug("Success return");
    ret_val = SORM_OK;

DB_FINALIZE :

    ret = sqlite3_finalize(stmt_handle);
    if(ret != SQLITE_OK)
    {
        log_debug("sqlite3_finalize error : %s", 
                sqlite3_errmsg(conn->sqlite3_handle));
        return SORM_DB_ERROR;

    }
    return ret_val;
    
}
