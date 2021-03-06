/****************************************************************************
 *       Filename:  c_generate.c
 *
 *    Description:  generate for a sorm c file
 *
 *        Version:  1.0
 *        Created:  05/22/2013 03:21:30 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *          Email:  never.wencan@gmail.com
 ***************************************************************************/
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "c_generate.h"
#include "generate.h"
#include "sorm.h"
#include "memory.h"

#define _str(string) ((string) == NULL ? "NULL" : (string))

static void c_generate_column_desc(
        FILE *file, const sorm_table_descriptor_t* table_desc)
{
    int i;
    char *upper_column_name, *upper_table_name;
    sorm_column_descriptor_t *column_desc;

    fprintf(file, "static sorm_column_descriptor_t "
            "%s_columns_descriptor[%d] =\n{\n", 
            table_desc->name, table_desc->columns_num);

    upper_table_name = _malloc(NULL, strlen(table_desc->name) + 1);
    case_lower2upper(table_desc->name, upper_table_name);

    for(i = 0; i < table_desc->columns_num; i ++)
    {
        column_desc = &table_desc->columns[i];
        upper_column_name = _malloc(NULL, strlen(column_desc->name) + 1);
        case_lower2upper(column_desc->name, upper_column_name);

        fprintf(file, INDENT "{\n"         
                INDENT_TWICE "\"%s\",\n"        /* name                 */
                INDENT_TWICE "%d,\n"            /* index                */
                INDENT_TWICE "%s,\n"            /* type                 */
                INDENT_TWICE "%s,\n"            /* constraint           */
                INDENT_TWICE "%s,\n"            /* mem                  */
                INDENT_TWICE "%s__%s__MAX_LEN,\n"    /* text_max_len         */
                INDENT_TWICE "SORM_OFFSET(%s_t, %s),\n"
                /* offset               */
                INDENT_TWICE "%d,\n"            /* is_foreign_key       */
                INDENT_TWICE "\"%s\",\n"        /* foreign_table_name   */
                INDENT_TWICE "\"%s\",\n"        /* foreign_column_name  */
                INDENT "},\n",
                column_desc->name, i, sorm_strtype(column_desc->type),
                sorm_strconstraint(column_desc->constraint), 
                sorm_strmem(column_desc->mem), 
                upper_table_name, upper_column_name,
                table_desc->name, column_desc->name, 
                column_desc->is_foreign_key, _str(column_desc->foreign_table_name),
                _str(column_desc->foreign_column_name));

        _free(NULL, upper_column_name);
        upper_column_name = NULL;
    }

    _free(NULL, upper_table_name);
    fprintf(file, "};\n\n");
}

static void c_generate_device_desc(
        FILE *file, const sorm_table_descriptor_t *table_desc)
{
    fprintf(file, "sorm_table_descriptor_t "
            "%s_table_descriptor =\n{\n", table_desc->name);

    fprintf(file, INDENT "\"%s\",\n"                /* name */
            INDENT "SORM_SIZE(%s_t),\n"             /* size */   
            INDENT "%d,\n"                          /* columns_num */
            INDENT "%d,\n"                          /* Primary Key index */
            INDENT "\"%s\",\n"                   /* create sql */
            INDENT "%s_columns_descriptor,\n",      /* columns */
            table_desc->name, table_desc->name, table_desc->columns_num,
            table_desc->PK_index, table_desc->create_sql,
            table_desc->name);

    fprintf(file, "};\n\n");
}

static void c_generate_func_to_string(
        FILE *file, const sorm_table_descriptor_t* table_desc)
{
    fprintf(file, "char* %s_to_string(\n"
            INDENT_TWICE "%s_t *%s, char *string, int len)\n{\n", 
            table_desc->name, table_desc->name, table_desc->name);
    fprintf(file, INDENT "return sorm_to_string((sorm_table_descriptor_t*)%s, "
            "string, len);\n", table_desc->name);
    fprintf(file, "}\n\n");
}

static void c_generate_func_get_desc(
        FILE *file, const sorm_table_descriptor_t* table_desc)
{
    fprintf(file, "sorm_table_descriptor_t* %s_get_desc()\n{\n", table_desc->name);
    fprintf(file, INDENT "return &%s_table_descriptor;\n", table_desc->name);
    fprintf(file, "}\n\n");
}

static void c_generate_func_new(
        FILE *file, const sorm_table_descriptor_t *table_desc)
{
    fprintf(file, "%s_t* %s_new(const sorm_allocator_t *allocator)\n{\n", table_desc->name, table_desc->name);
    fprintf(file, INDENT "return (%s_t *)sorm_new(allocator, &%s_table_descriptor);\n", 
            table_desc->name, table_desc->name);
    fprintf(file, "}\n\n");
    
    fprintf(file, "%s_t* %s_new_array(const sorm_allocator_t *allocator, int num)\n{\n", table_desc->name, table_desc->name);
    fprintf(file, INDENT "return (%s_t *)sorm_new_array(allocator, &%s_table_descriptor, num);\n", 
            table_desc->name, table_desc->name);
    fprintf(file, "}\n\n");
}

static void c_generate_func_free(
        FILE *file, const sorm_table_descriptor_t *table_desc)
{
    fprintf(file, "void %s_free(const sorm_allocator_t *allocator, %s_t *%s)\n{\n", 
            table_desc->name, table_desc->name, table_desc->name);
    fprintf(file, INDENT "sorm_free(allocator, (sorm_table_descriptor_t *)%s);\n",
            table_desc->name);
    fprintf(file, "}\n\n");

    fprintf(file, "void %s_free_array(const sorm_allocator_t *allocator, %s_t *%s, int n)\n{\n", 
            table_desc->name, table_desc->name, table_desc->name);
    fprintf(file, INDENT "sorm_free_array(allocator, (sorm_table_descriptor_t *)%s, n);\n",
            table_desc->name);
    fprintf(file, "}\n\n");
}

static void c_generate_func_create_table(
        FILE *file, const sorm_table_descriptor_t *table_desc, char *in_file_name)
{
    FILE *in_file;
    char sql_stmt[SQL_STMT_MAX_LEN + 1];

    in_file = fopen(in_file_name, "r");
    if(in_file == NULL)
    {
        printf("fopen file(%s) error : %s.\n", in_file_name, strerror(errno));
        return;
    }
    
    if(fgets(sql_stmt, SQL_STMT_MAX_LEN + 1, in_file) == NULL)
    {
        printf("read from file(%s) error : %s.\n", in_file_name, strerror(errno));
        return;
    }

    fclose(in_file);

    sql_stmt[strlen(sql_stmt) - 1] = '\0';

    fprintf(file, "int %s_create_table(const sorm_connection_t *conn)\n{\n", 
            table_desc->name);
    fprintf(file, INDENT "char *sql_stmt = \"%s\";\n\n", sql_stmt);
    fprintf(file, INDENT "return sorm_run_stmt(conn, sql_stmt, SORM_RWLOCK_WRITE);\n");
    fprintf(file, "}\n\n");

    return;

}

static void c_generate_func_delete_table(
        FILE *file, const sorm_table_descriptor_t *table_desc)
{
    fprintf(file, "int %s_delete_table(const sorm_connection_t *conn)\n{\n", 
            table_desc->name);
    fprintf(file, INDENT "return sorm_delete_table(conn, &%s_table_descriptor);",
            table_desc->name);
    fprintf(file, "}\n\n");
}

static void c_generate_func_save(
        FILE *file, const sorm_table_descriptor_t *table_desc)
{
    fprintf(file, "int %s_insert(sorm_connection_t *conn, %s_t *%s)\n{\n",
            table_desc->name, table_desc->name, table_desc->name);
    fprintf(file, INDENT "return sorm_insert"
            "(conn, (sorm_table_descriptor_t*)%s);\n", table_desc->name);
    fprintf(file, "}\n\n");
    fprintf(file, "int %s_replace(sorm_connection_t *conn, %s_t *%s)\n{\n",
            table_desc->name, table_desc->name, table_desc->name);
    fprintf(file, INDENT "return sorm_replace"
            "(conn, (sorm_table_descriptor_t*)%s);\n", table_desc->name);
    fprintf(file, "}\n\n");
}

static void c_generate_func_update(
        FILE *file, const sorm_table_descriptor_t *table_desc)
{
    fprintf(file, 
            "int %s_update(sorm_connection_t *conn, %s_t *%s)\n{\n",
            table_desc->name, table_desc->name, table_desc->name);
    fprintf(file, INDENT "return sorm_update"
            "(conn, (sorm_table_descriptor_t*)%s);\n", 
            table_desc->name);
    fprintf(file, "}\n\n");
}

static void c_generate_func_set_mem(
        FILE *file, const sorm_table_descriptor_t *table_desc)
{
    int i;

    for(i = 0; i < table_desc->columns_num; i ++)
    {
        switch(table_desc->columns[i].type)
        {
            case SORM_TYPE_INT :
                fprintf(file, "int %s_set_%s(%s_t *%s, int %s)\n{\n", 
                        table_desc->name, table_desc->columns[i].name, 
                        table_desc->name, table_desc->name, 
                        table_desc->columns[i].name);
                fprintf(file, INDENT "return sorm_set_column_value("
                        "(sorm_table_descriptor_t*)%s, %d, &%s, 0);\n",
                        table_desc->name, i, table_desc->columns[i].name);
                break;
            case SORM_TYPE_INT64 :
                fprintf(file, "int %s_set_%s(%s_t *%s, int64_t %s)\n{\n", 
                        table_desc->name, table_desc->columns[i].name, 
                        table_desc->name, table_desc->name, 
                        table_desc->columns[i].name);
                fprintf(file, INDENT "return sorm_set_column_value("
                        "(sorm_table_descriptor_t*)%s, %d, &%s, 0);\n",
                        table_desc->name, i, table_desc->columns[i].name);
                break;
            case SORM_TYPE_TEXT :
                fprintf(file, "int %s_set_%s(%s_t *%s, const char* %s)\n{\n", 
                        table_desc->name, table_desc->columns[i].name, 
                        table_desc->name, table_desc->name, 
                        table_desc->columns[i].name);
                fprintf(file, INDENT "return sorm_set_column_value("
                        "(sorm_table_descriptor_t*)%s, %d, %s, 0);\n",
                        table_desc->name, i, table_desc->columns[i].name);
                break;
            case SORM_TYPE_DOUBLE :
                fprintf(file, "int %s_set_%s(%s_t *%s, double %s)\n{\n", 
                        table_desc->name, table_desc->columns[i].name, 
                        table_desc->name, table_desc->name, 
                        table_desc->columns[i].name);
                fprintf(file, INDENT "return sorm_set_column_value("
                        "(sorm_table_descriptor_t*)%s, %d, &%s, 0);\n",
                        table_desc->name, i, table_desc->columns[i].name);
                break;
            case SORM_TYPE_BLOB :
                fprintf(file, "int %s_set_%s(%s_t *%s, const void* %s, int len)\n{\n", 
                        table_desc->name, table_desc->columns[i].name, 
                        table_desc->name, table_desc->name, 
                        table_desc->columns[i].name);
                fprintf(file, INDENT "return sorm_set_column_value("
                        "(sorm_table_descriptor_t*)%s, %d, %s, len);\n",
                        table_desc->name, i, table_desc->columns[i].name);
                break;
            default :
                printf("Invalid SORM_TYPE.\n");
                exit(0);
        }
        fprintf(file, "}\n\n");
    }
}

static void c_generate_func_delete(
        FILE *file, const sorm_table_descriptor_t *table_desc)
{
    int i;

    fprintf(file, "int %s_delete(const sorm_connection_t *conn, "
            "const %s_t *%s)\n{\n",
            table_desc->name, table_desc->name, table_desc->name);
    fprintf(file, INDENT "return sorm_delete"
            "(conn, (sorm_table_descriptor_t *)%s);\n", table_desc->name);
    fprintf(file, "}\n\n");
    
    fprintf(file, "int %s_delete_by(const sorm_connection_t *conn, "
            "const char *filter)\n{\n", table_desc->name);
    fprintf(file, INDENT "return sorm_delete_by"
            "(conn, &%s_table_descriptor, filter);\n", table_desc->name);
    fprintf(file, "}\n\n");

    for(i = 0; i < table_desc->columns_num; i ++)
    {
        if(table_desc->columns[i].constraint == SORM_CONSTRAINT_PK 
                || table_desc->columns[i].constraint == SORM_CONSTRAINT_PK_ASC
                || table_desc->columns[i].constraint == SORM_CONSTRAINT_PK_DESC
                || table_desc->columns[i].constraint == SORM_CONSTRAINT_UNIQUE)
        {
            switch(table_desc->columns[i].type)
            {
                case SORM_TYPE_INT :
                    fprintf(file, "int %s_delete_by_%s(const sorm_connection_t "
                            "*conn, int %s)\n{\n", table_desc->name, 
                            table_desc->columns[i].name, 
                            table_desc->columns[i].name);
                    fprintf(file, INDENT "return sorm_delete_by_column(\n"
                            INDENT_TRIPLE "conn, &%s_table_descriptor, "
                            "%d, (void *)&%s);\n",
                            table_desc->name, i, table_desc->columns[i].name);
                    break;
                case SORM_TYPE_INT64 :
                    fprintf(file, "int %s_delete_by_%s(const sorm_connection_t "
                            "*conn, int64_t %s)\n{\n", table_desc->name, 
                            table_desc->columns[i].name, 
                            table_desc->columns[i].name);
                    fprintf(file, INDENT "return sorm_delete_by_column(\n"
                            INDENT_TRIPLE "conn, &%s_table_descriptor, "
                            "%d, (void *)&%s);\n",
                            table_desc->name, i, table_desc->columns[i].name);
                    break;
                case SORM_TYPE_TEXT :
                    fprintf(file, "int %s_delete_by_%s(const sorm_connection_t "
                            "*conn, char* %s)\n{\n", table_desc->name,
                            table_desc->columns[i].name, 
                            table_desc->columns[i].name);
                    fprintf(file, INDENT "return sorm_delete_by_column(\n"
                            INDENT_TRIPLE "conn, &%s_table_descriptor, "
                            "%d, (void *)%s);\n",
                            table_desc->name, i, table_desc->columns[i].name);
                    break;
                case SORM_TYPE_DOUBLE :
                    fprintf(file, "int %s_delete_by_%s(const sorm_connection_t "
                            "*conn, double %s)\n{\n", table_desc->name,
                            table_desc->columns[i].name, 
                            table_desc->columns[i].name);
                    fprintf(file, INDENT "return sorm_delete_by_column(\n"
                            INDENT_TRIPLE "conn, &%s_table_descriptor, "
                            "%d, (void *)&%s);\n",
                            table_desc->name, i, table_desc->columns[i].name);
                    break;
                case SORM_TYPE_BLOB :
                    break;
                default :
                    printf("Invalid SORM_TYPE.\n");
                    exit(0);
            }
            fprintf(file, "}\n\n");
        }
    }
}

static void c_generate_func_select(
        FILE *file, const sorm_table_descriptor_t *table_desc)
{
    int i;
    int foreign_table_name_len, foreign_column_name_len;
    char *upper_foreign_table_name, *upper_foreign_column_name;
    char *upper_column_name, *upper_table_name;
    sorm_column_descriptor_t *column_desc;

    /* by member */
    for(i = 0; i < table_desc->columns_num; i ++)
    {
        if(table_desc->columns[i].constraint == SORM_CONSTRAINT_PK 
                || table_desc->columns[i].constraint == SORM_CONSTRAINT_PK_ASC
                || table_desc->columns[i].constraint == SORM_CONSTRAINT_PK_DESC
                || table_desc->columns[i].constraint == SORM_CONSTRAINT_UNIQUE)
        {
            switch(table_desc->columns[i].type) {
                case SORM_TYPE_INT :
                    fprintf(file, "int %s_select_by_%s(\n"
                            INDENT_TWICE "const sorm_connection_t *conn,\n"
                            INDENT_TWICE "const sorm_allocator_t *allocator,\n"
                            INDENT_TWICE "const char *column_names, int %s,\n"
                            INDENT_TWICE "%s_t **%s)\n{\n", 
                            table_desc->name, table_desc->columns[i].name, 
                            table_desc->columns[i].name,
                            table_desc->name, table_desc->name);
                    fprintf(file, INDENT "return sorm_select_one_by_column(\n"
                            INDENT_TRIPLE "conn, allocator, &%s_table_descriptor, "
                            "column_names,\n"
                            INDENT_TRIPLE "%d, (void *)&%s, "
                            "(sorm_table_descriptor_t **)%s);\n", 
                            table_desc->name, i, table_desc->columns[i].name, 
                            table_desc->name);
                    break;
                case SORM_TYPE_INT64 :
                    fprintf(file, "int %s_select_by_%s(\n"
                            INDENT_TWICE "const sorm_connection_t *conn,\n"
                            INDENT_TWICE "const sorm_allocator_t *allocator,\n"
                            INDENT_TWICE "const char *column_names, int64_t %s,\n"
                            INDENT_TWICE "%s_t **%s)\n{\n", 
                            table_desc->name, table_desc->columns[i].name, 
                            table_desc->columns[i].name,
                            table_desc->name, table_desc->name);
                    fprintf(file, INDENT "return sorm_select_one_by_column(\n"
                            INDENT_TRIPLE "conn, allocator, &%s_table_descriptor, "
                            "column_names,\n"
                            INDENT_TRIPLE "%d, (void *)&%s, "
                            "(sorm_table_descriptor_t **)%s);\n", 
                            table_desc->name, i, table_desc->columns[i].name, 
                            table_desc->name);
                    break;
                case SORM_TYPE_TEXT :
                    fprintf(file, "int %s_select_by_%s(\n"
                            INDENT_TWICE "const sorm_connection_t *conn,\n"
                            INDENT_TWICE "const sorm_allocator_t *allocator,\n"
                            INDENT_TWICE "const char *column_names," 
                            "const char* %s,\n"
                            INDENT_TWICE "%s_t **%s)\n{\n", 
                            table_desc->name, table_desc->columns[i].name, 
                            table_desc->columns[i].name,
                            table_desc->name, table_desc->name);
                    fprintf(file, INDENT "return sorm_select_one_by_column(\n"
                            INDENT_TRIPLE "conn, allocator, &%s_table_descriptor, "
                            "column_names,\n"
                            INDENT_TRIPLE "%d, (void *)%s, "
                            "(sorm_table_descriptor_t **)%s);\n", 
                            table_desc->name, i, table_desc->columns[i].name, 
                            table_desc->name);
                    break;
                case SORM_TYPE_DOUBLE :
                    fprintf(file, "int %s_select_by_%s(\n"
                            INDENT_TWICE "const sorm_connection_t *conn,\n"
                            INDENT_TWICE "const sorm_allocator_t *allocator,\n"
                            INDENT_TWICE "const char *column_names, "
                            "double* %s,\n"
                            INDENT_TWICE "%s_t **%s)\n{\n", 
                            table_desc->name, table_desc->columns[i].name, 
                            table_desc->columns[i].name,
                            table_desc->name, table_desc->name);
                    fprintf(file, INDENT "return sorm_select_one_by_column(\n"
                            INDENT_TRIPLE "conn, allocator, &%s_table_descriptor, "
                            "column_names,\n"
                            INDENT_TRIPLE "%d, (void *)&%s, "
                            "(sorm_table_descriptor_t **)%s);\n", 
                            table_desc->name, i, table_desc->columns[i].name, 
                            table_desc->name);
                    break;
                case SORM_TYPE_BLOB :
                    break;
                default :
                    printf("Invalid SORM_TYPE.\n");
                    exit(0);
            }
            fprintf(file, "}\n\n");
        }
    }

    fprintf(file, "int %s_select_some_array_by(\n"
            INDENT_TWICE "const sorm_connection_t *conn,\n"
            INDENT_TWICE "const sorm_allocator_t *allocator,\n"
            INDENT_TWICE "const char *column_names, const char *filter,\n" 
            INDENT_TWICE "int *n, %s_t **%ss_array)\n{\n"
            INDENT       "return sorm_select_some_array_by(\n"
            INDENT_TRIPLE "conn, allocator, &%s_table_descriptor, column_names,\n"
            INDENT_TRIPLE "filter, n, "
            "(sorm_table_descriptor_t **)%ss_array);\n}\n\n",
            table_desc->name, table_desc->name, table_desc->name, 
            table_desc->name, table_desc->name);
    fprintf(file, "int %s_select_some_list_by(\n"
            INDENT_TWICE "const sorm_connection_t *conn,\n"
            INDENT_TWICE "const sorm_allocator_t *allocator,\n"
            INDENT_TWICE "const char *column_names, const char *filter,\n" 
            INDENT_TWICE "int *n, sorm_list_t **%ss_list_head)\n{\n"
            INDENT       "return sorm_select_some_list_by(\n"
            INDENT_TRIPLE "conn, allocator, &%s_table_descriptor, column_names,\n"
            INDENT_TRIPLE "filter, n, %ss_list_head);\n}\n\n",
            table_desc->name, table_desc->name, table_desc->name, 
            table_desc->name);
    fprintf(file, "int %s_select_all_array_by(\n"
            INDENT_TWICE "const sorm_connection_t *conn,\n"
            INDENT_TWICE "const sorm_allocator_t *allocator,\n"
            INDENT_TWICE "const char *column_names, const char *filter,\n"
            INDENT_TWICE "int *n, %s_t **%ss_array)\n{\n"
            INDENT       "return sorm_select_all_array_by(\n"
            INDENT_TRIPLE "conn, allocator, &%s_table_descriptor, column_names,\n"
            INDENT_TRIPLE "filter, n, "
            "(sorm_table_descriptor_t **)%ss_array);\n}\n\n",
            table_desc->name, table_desc->name, table_desc->name,  
            table_desc->name, table_desc->name);
    fprintf(file, "int %s_select_all_list_by(\n"
            INDENT_TWICE "const sorm_connection_t *conn,\n"
            INDENT_TWICE "const sorm_allocator_t *allocator,\n"
            INDENT_TWICE "const char *column_names, const char *filter,\n" 
            INDENT_TWICE "int *n, sorm_list_t **%ss_list_head)\n{\n"
            INDENT       "return sorm_select_all_list_by(\n"
            INDENT_TRIPLE "conn, allocator, &%s_table_descriptor, column_names,\n"
            INDENT_TRIPLE "filter, n, %ss_list_head);\n}\n\n",
            table_desc->name, table_desc->name, 
            table_desc->name, table_desc->name);
    fprintf(file, "int %s_select_iterate_by_open(\n"
            INDENT_TWICE "const sorm_connection_t *conn,\n"
            INDENT_TWICE "const sorm_allocator_t *allocator,\n"
            INDENT_TWICE "const char *column_names, const char *filter,\n" 
            INDENT_TWICE "sorm_iterator_t **iterator)\n{\n"
            INDENT       "return sorm_select_iterate_by_open(\n"
            INDENT_TRIPLE "conn, allocator, &%s_table_descriptor, column_names,\n"
            INDENT_TRIPLE "filter, iterator);\n}\n\n",
            table_desc->name, table_desc->name);
    fprintf(file, "int %s_select_iterate_by(\n"
            INDENT_TWICE "sorm_iterator_t *iterator, %s_t **%s)\n{\n"
            INDENT       "return sorm_select_iterate_by(\n"
            INDENT_TRIPLE "iterator, (sorm_table_descriptor_t **)%s);\n}\n\n",
            table_desc->name, table_desc->name, 
            table_desc->name, table_desc->name);

    /* select by foreign key */
    //upper_table_name = _malloc(NULL, strlen(table_desc->name) + 1);
    //case_lower2upper(table_desc->name, upper_table_name);
    //for(i = 0; i < table_desc->columns_num; i ++)
    //{
    //    if(table_desc->columns[i].is_foreign_key == 1)
    //    {
    //        column_desc = &table_desc->columns[i];
    //        upper_column_name = _malloc(NULL, strlen(column_desc->name) + 1);
    //        case_lower2upper(column_desc->name, upper_column_name);

    //        foreign_table_name_len = strlen(column_desc->foreign_table_name);
    //        upper_foreign_table_name = _malloc(NULL, foreign_table_name_len + 1);
    //        case_lower2upper(column_desc->foreign_table_name, upper_foreign_table_name);
    //        
    //        foreign_column_name_len = strlen(column_desc->foreign_column_name);
    //        upper_foreign_column_name = _malloc(NULL, foreign_column_name_len + 1);
    //        case_lower2upper(column_desc->foreign_column_name, upper_foreign_column_name);

    //        fprintf(file, "int %s_select_some_array_by_%s(\n"
    //                INDENT_TWICE "const sorm_connection_t *conn,\n"
    //                INDENT_TWICE "const sorm_allocator_t *allocator,\n"
    //                INDENT_TWICE "const char *column_names, const char *filter,\n" 
    //                INDENT_TWICE "int *n, %s_t **%ss_array)\n{\n"
    //                INDENT     "return sorm_select_some_array_by_join(conn, allocator, column_names,\n"
    //                INDENT_TRIPLE "&%s_table_descriptor, COLUMN__%s__%s,\n"
    //                INDENT_TRIPLE "DESC__%s, COLUMN__%s__%s, \n"
    //                INDENT_TRIPLE "SORM_INNER_JOIN, filter, n, \n"
    //                INDENT_TRIPLE "(sorm_table_descriptor_t **)%ss_array, NULL);\n}\n\n",
    //                table_desc->name, column_desc->foreign_table_name,
    //                table_desc->name, table_desc->name, table_desc->name, 
    //                upper_table_name, upper_column_name, upper_foreign_table_name,  
    //                upper_foreign_table_name, upper_foreign_column_name, table_desc->name);
    //        fprintf(file, "int %s_select_some_list_by_%s(\n"
    //                INDENT_TWICE "const sorm_connection_t *conn,\n"
    //                INDENT_TWICE "const sorm_allocator_t *allocator,\n"
    //                INDENT_TWICE"const char *column_names, const char *filter,\n" 
    //                INDENT_TWICE"int *n, sorm_list_t **%ss_list_head)\n{\n"
    //                INDENT     "return sorm_select_some_list_by_join(conn, allocator, column_names,\n"
    //                INDENT_TRIPLE "&%s_table_descriptor, COLUMN__%s__%s, \n"
    //                INDENT_TRIPLE "DESC__%s, COLUMN__%s__%s, \n"
    //                INDENT_TRIPLE "SORM_INNER_JOIN, filter, n, %ss_list_head, NULL);\n}\n\n",
    //                table_desc->name, column_desc->foreign_table_name,
    //                table_desc->name, table_desc->name, 
    //                upper_table_name, upper_column_name, upper_foreign_table_name,  
    //                upper_foreign_table_name, upper_foreign_column_name, table_desc->name);
    //        fprintf(file, "int %s_select_all_array_by_%s(\n"
    //                INDENT_TWICE "const sorm_connection_t *conn,\n"
    //                INDENT_TWICE "const sorm_allocator_t *allocator,\n"
    //                INDENT_TWICE "const char *column_names, const char *filter,\n"
    //                INDENT_TWICE "int *n, %s_t **%ss_array)\n{\n"
    //                INDENT     "return sorm_select_all_array_by_join(conn, allocator, column_names,\n"
    //                INDENT_TRIPLE "&%s_table_descriptor, COLUMN__%s__%s,\n"
    //                INDENT_TRIPLE "DESC__%s, COLUMN__%s__%s, \n"
    //                INDENT_TRIPLE "SORM_INNER_JOIN, filter, n, \n"
    //                INDENT_TRIPLE "(sorm_table_descriptor_t **)%ss_array, NULL);\n}\n\n",
    //                table_desc->name, column_desc->foreign_table_name,
    //                table_desc->name, table_desc->name, table_desc->name, 
    //                upper_table_name, upper_column_name, upper_foreign_table_name,  
    //                upper_foreign_table_name, upper_foreign_column_name, table_desc->name);
    //        fprintf(file, "int %s_select_all_list_by_%s(\n"
    //                INDENT_TWICE "const sorm_connection_t *conn,\n"
    //                INDENT_TWICE "const sorm_allocator_t *allocator,\n"
    //                INDENT_TWICE "const char *column_names, const char *filter,\n" 
    //                INDENT_TWICE "int *n, sorm_list_t **%ss_list_head)\n{\n"
    //                INDENT     "return sorm_select_all_list_by_join(conn, allocator, column_names,\n"
    //                INDENT_TRIPLE "&%s_table_descriptor, COLUMN__%s__%s,\n"
    //                INDENT_TRIPLE "DESC__%s, COLUMN__%s__%s, \n"
    //                INDENT_TRIPLE "SORM_INNER_JOIN, filter, n, %ss_list_head, NULL);\n}\n\n",
    //                table_desc->name, column_desc->foreign_table_name,
    //                table_desc->name, table_desc->name, 
    //                upper_table_name, upper_column_name, upper_foreign_table_name,  
    //                upper_foreign_table_name, upper_foreign_column_name, table_desc->name);

    //        _free(NULL, upper_foreign_column_name);
    //        _free(NULL, upper_foreign_table_name);
    //    }
    //}
    _free(NULL, upper_table_name);
}

static void c_generate_func_index(
        FILE *file, const sorm_table_descriptor_t *table_desc)
{
    fprintf(file, "int %s_create_index(\n"
            INDENT_TWICE "const sorm_connection_t *conn, char *columns_name)\n{\n"
            INDENT     "return sorm_create_index(conn, &%s_table_descriptor, "
            "columns_name);\n}\n\n",
            table_desc->name, table_desc->name);
    fprintf(file, "int %s_drop_index(\n"
            INDENT_TWICE "const sorm_connection_t *conn, char *columns_name)\n{\n"
            INDENT     "return sorm_drop_index(conn, columns_name);\n}\n\n",
            table_desc->name);
}

void c_generate(
        const sorm_table_descriptor_t *table_desc, char *in_file_name)
{
    FILE *file;
    char *file_name, *header_file_name, *foreign_header_file_name;
    int table_name_len, foreign_table_name_len;
    int i;

    table_name_len = strlen(table_desc->name);
    file_name = _malloc(NULL, table_name_len + 8);
    sprintf(file_name, "%s_sorm.c", table_desc->name);
    header_file_name = _malloc(NULL, table_name_len + 8);
    sprintf(header_file_name, "%s_sorm.h", table_desc->name);

    file = fopen(file_name, "w");
    if(file == NULL)
    {
        printf("fopen file(%s) error : %s.\n", file_name, strerror(errno));
    }

    fprintf(file, "#include \"%s\"\n", header_file_name);
    for(i = 0; i < table_desc->columns_num; i ++)
    {
        if(table_desc->columns[i].is_foreign_key == 1)
        {
            foreign_table_name_len = strlen(
                    table_desc->columns[i].foreign_table_name);
            foreign_header_file_name = _malloc(NULL, foreign_table_name_len + 8);
            sprintf(foreign_header_file_name, "%s_sorm.h", 
                    table_desc->columns[i].foreign_table_name);
            fprintf(file, "#include \"%s\"\n", foreign_header_file_name);
            _free(NULL, foreign_header_file_name);
        }
    }

    c_generate_column_desc(file, table_desc);
    c_generate_device_desc(file, table_desc);
    /* functions */
    c_generate_func_to_string(file, table_desc);
    c_generate_func_get_desc(file, table_desc);
    c_generate_func_new(file, table_desc);
    c_generate_func_free(file, table_desc);
    c_generate_func_create_table(file, table_desc, in_file_name);
    c_generate_func_delete_table(file, table_desc);
    c_generate_func_save(file, table_desc);
    c_generate_func_update(file, table_desc);
    c_generate_func_set_mem(file, table_desc);
    c_generate_func_delete(file, table_desc);
    c_generate_func_select(file, table_desc);
    c_generate_func_index(file, table_desc);
}
