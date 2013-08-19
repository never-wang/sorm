/****************************************************************************
 *       Filename:  header_generate.c
 *
 *    Description:  generate for a sorm header
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

#include "header_generate.h"
#include "generate.h"
#include "sorm.h"
#include "memory.h"

static void header_generate_struct(
        FILE *file, const sorm_table_descriptor_t* table_desc)
{
    int i;
    char *upper_column_name, *upper_table_name;

    fprintf(file, "typedef struct %s_s\n{\n"
            INDENT "sorm_table_descriptor_t table_desc;\n\n",
            table_desc->name);

    upper_table_name = sys_malloc(strlen(table_desc->name) + 1);
    case_lower2upper(table_desc->name, upper_table_name);

    for(i = 0; i < table_desc->columns_num; i ++)
    {
        if(table_desc->columns[i].type == SORM_TYPE_BLOB)
        {
            fprintf(file, INDENT "int         %s_len;\n",
                    table_desc->columns[i].name);
        }
        fprintf(file, INDENT "sorm_stat_t %s_stat;\n",
                table_desc->columns[i].name);

        switch(table_desc->columns[i].type)
        {
            case SORM_TYPE_INT :
                fprintf(file, INDENT "int         %s;\n\n", 
                        table_desc->columns[i].name);
                break;
            case SORM_TYPE_TEXT :
                if(table_desc->columns[i].mem == SORM_MEM_HEAP)
                {
                    fprintf(file, INDENT "char*       %s;\n\n",
                            table_desc->columns[i].name);
                }else if(table_desc->columns[i].mem == SORM_MEM_STACK)
                {
                    upper_column_name = sys_malloc(
                            strlen(table_desc->columns[i].name) + 1);
                    case_lower2upper(
                            table_desc->columns[i].name, upper_column_name);

                    fprintf(file, INDENT "char        %s[%s_%s_MAX_LEN + 1];\n\n",
                            table_desc->columns[i].name,
                            upper_table_name, upper_column_name);
                    sys_free(upper_column_name);
                    upper_column_name = NULL;
                }else
                {
                    error("Invalid SORM_MEM.");
                    exit(0);
                }
                break;
            case SORM_TYPE_DOUBLE :
                fprintf(file, INDENT "double      %s;\n\n",
                        table_desc->columns[i].name);
                break;
            case SORM_TYPE_BLOB :
                if(table_desc->columns[i].mem == SORM_MEM_HEAP)
                {
                    fprintf(file, INDENT "void*       %s;\n\n",
                            table_desc->columns[i].name);
                }else if(table_desc->columns[i].mem == SORM_MEM_STACK)
                {
                    upper_column_name = sys_malloc(
                            strlen(table_desc->columns[i].name) + 1);
                    case_lower2upper(
                            table_desc->columns[i].name, upper_column_name);

                    fprintf(file, INDENT "char        %s[%s_%s_MAX_LEN + 1];\n\n",
                            table_desc->columns[i].name,
                            upper_table_name, upper_column_name);
                    sys_free(upper_column_name);
                    upper_column_name = NULL;
                }else
                {
                    error("Invalid SORM_MEM.");
                    exit(0);
                }
                break;
            default :
                error("Invalid SORM_TYPE.");
                exit(0);
        }

    }

    sys_free(upper_table_name);
    fprintf(file, "} %s_t;\n\n", table_desc->name);
}

static void header_generate_func_get_desc(
        FILE *file, const sorm_table_descriptor_t* table_desc)
{
    fprintf(file, "sorm_table_descriptor_t* %s_get_desc();\n\n", table_desc->name);
}

static void header_generate_func_new(
        FILE *file, const sorm_table_descriptor_t *table_desc)
{
    fprintf(file, "%s_t* %s_new();\n\n", table_desc->name, table_desc->name);
}

static void header_generate_func_free(
        FILE *file, const sorm_table_descriptor_t *table_desc)
{
    fprintf(file, "void %s_free(%s_t *%s);\n\n", 
            table_desc->name, table_desc->name, table_desc->name);

    fprintf(file, "void %s_free_array(%s_t *%s, int n);\n\n", 
            table_desc->name, table_desc->name, table_desc->name);
}

static void header_generate_func_create_table(
        FILE *file, const sorm_table_descriptor_t *table_desc)
{
    fprintf(file, "int %s_create_table(const sorm_connection_t *conn);\n\n", 
            table_desc->name);
}

static void header_generate_func_delete_table(
        FILE *file, const sorm_table_descriptor_t *table_desc)
{
    fprintf(file, "int %s_delete_table(const sorm_connection_t *conn);\n\n", 
            table_desc->name);
}

static void header_generate_func_save(
        FILE *file, const sorm_table_descriptor_t *table_desc)
{
    fprintf(file, "int %s_save(sorm_connection_t *conn, %s_t *%s);\n\n",
            table_desc->name, table_desc->name, table_desc->name);
}

static void header_generate_func_set_mem(
        FILE *file, const sorm_table_descriptor_t *table_desc)
{
    int i;

    for(i = 0; i < table_desc->columns_num; i ++)
    {
        switch(table_desc->columns[i].type)
        {
            case SORM_TYPE_INT :
                fprintf(file, "int %s_set_%s(%s_t *%s, int %s);\n", 
                        table_desc->name, table_desc->columns[i].name, 
                        table_desc->name, table_desc->name, 
                        table_desc->columns[i].name);
                break;
            case SORM_TYPE_TEXT :
                fprintf(file, "int %s_set_%s(%s_t *%s, const char* %s);\n", 
                        table_desc->name, table_desc->columns[i].name, 
                        table_desc->name, table_desc->name, 
                        table_desc->columns[i].name);
                break;
            case SORM_TYPE_DOUBLE :
                fprintf(file, "int %s_set_%s(%s_t *%s, double %s);\n", 
                        table_desc->name, table_desc->columns[i].name, 
                        table_desc->name, table_desc->name, 
                        table_desc->columns[i].name);
            case SORM_TYPE_BLOB :
                fprintf(file, "int %s_set_%s(%s_t *%s, const void* %s, int len);\n", 
                        table_desc->name, table_desc->columns[i].name, 
                        table_desc->name, table_desc->name, 
                        table_desc->columns[i].name);
                break;
                break;
            default :
                error("Invalid SORM_TYPE.");
                exit(0);
        }
    }

    fprintf(file, "\n");
}

static void header_generate_func_delete(
        FILE *file, const sorm_table_descriptor_t *table_desc)
{
    int i;

    fprintf(file, "int %s_delete(const sorm_connection_t *conn, "
            "const %s_t *%s);\n\n",
            table_desc->name, table_desc->name, table_desc->name);

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
                            "*conn, int %s);\n", table_desc->name, 
                            table_desc->columns[i].name, 
                            table_desc->columns[i].name);
                    break;
                case SORM_TYPE_TEXT :
                    fprintf(file, "int %s_delete_by_%s(const sorm_connection_t "
                            "*conn, char* %s);\n", table_desc->name,
                            table_desc->columns[i].name, 
                            table_desc->columns[i].name);
                    break;
                case SORM_TYPE_DOUBLE :
                    fprintf(file, "int %s_delete_by_%s(const sorm_connection_t "
                            "*conn, double %s);\n", table_desc->name,
                            table_desc->columns[i].name, 
                            table_desc->columns[i].name);
                    break;
                case SORM_TYPE_BLOB :
                    break;
                default :
                    error("Invalid SORM_TYPE.");
                    exit(0);
            }
        }
    }
    fprintf(file, "\n");
}

static void header_generate_func_select(
        FILE *file, const sorm_table_descriptor_t *table_desc)
{
    int i;

    /* by member */
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
                    fprintf(file, "int %s_select_by_%s(\n"
                            INDENT "const sorm_connection_t *conn,\n"
                            INDENT "const char *column_names, int %s,\n"
                            INDENT "%s_t **%s);\n", table_desc->name, 
                            table_desc->columns[i].name, 
                            table_desc->columns[i].name, 
                            table_desc->name, table_desc->name);
                    break;
                case SORM_TYPE_TEXT :
                    fprintf(file, "int %s_select_by_%s(\n"
                            INDENT "const sorm_connection_t *conn,\n"
                            INDENT "const char *column_names, const char* %s,\n"
                            INDENT "%s_t **%s);\n", table_desc->name, 
                            table_desc->columns[i].name, 
                            table_desc->columns[i].name, 
                            table_desc->name, table_desc->name);
                    break;
                case SORM_TYPE_DOUBLE :
                    fprintf(file, "int %s_select_by_%s(\n"
                            INDENT "const sorm_connection_t *conn,\n"
                            INDENT "const char *column_names, double* %s,\n"
                            INDENT "%s_t **%s);\n", table_desc->name, 
                            table_desc->columns[i].name, 
                            table_desc->columns[i].name, 
                            table_desc->name, table_desc->name);
                    break;
                case SORM_TYPE_BLOB :
                    break;
                default :
                    error("Invalid SORM_TYPE.");
                    exit(0);
            }
        }
    }
    fprintf(file, "\n");

    fprintf(file, "int %s_select_some_array_by(\n"
            INDENT_TWICE "const sorm_connection_t *conn,\n"
            INDENT_TWICE "const char *column_names, const char *filter,\n" 
            INDENT_TWICE "int *n, %s_t **%ss_array);\n"
            "int %s_select_some_list_by(\n"
            INDENT_TWICE "const sorm_connection_t *conn,\n"
            INDENT_TWICE"const char *column_names, const char *filter,\n" 
            INDENT_TWICE"int *n, sorm_list_t **%ss_list_head);\n"
            "int %s_select_all_array_by(\n"
            INDENT_TWICE "const sorm_connection_t *conn,\n"
            INDENT_TWICE "const char *column_names, const char *filter,\n"
            INDENT_TWICE "int *n, %s_t **%ss_array);\n"
            "int %s_select_all_list_by(\n"
            INDENT_TWICE "const sorm_connection_t *conn,\n"
            INDENT_TWICE "const char *column_names, const char *filter,\n" 
            INDENT_TWICE "int *n, sorm_list_t **%ss_list_head);\n\n",
            table_desc->name, table_desc->name, 
            table_desc->name, table_desc->name,
            table_desc->name, table_desc->name, 
            table_desc->name, table_desc->name,
            table_desc->name, table_desc->name); 

    /* select by foreign key */
    for(i = 0; i < table_desc->columns_num; i ++)
    {
        if(table_desc->columns[i].is_foreign_key == 1)
        {
            fprintf(file, "int %s_select_some_array_by_%s(\n"
                    INDENT_TWICE "const sorm_connection_t *conn,\n"
                    INDENT_TWICE "const char *column_names, const char *filter,\n" 
                    INDENT_TWICE "int *n, %s_t **%ss_array);\n",
                    table_desc->name, table_desc->columns[i].foreign_table_name,
                    table_desc->name, table_desc->name);
            fprintf(file, "int %s_select_some_list_by_%s(\n"
                    INDENT_TWICE "const sorm_connection_t *conn,\n"
                    INDENT_TWICE"const char *column_names, const char *filter,\n" 
                    INDENT_TWICE"int *n, sorm_list_t **%ss_list_head);\n",
                    table_desc->name, table_desc->columns[i].foreign_table_name,
                    table_desc->name);
            fprintf(file, "int %s_select_all_array_by_%s(\n"
                    INDENT_TWICE "const sorm_connection_t *conn,\n"
                    INDENT_TWICE "const char *column_names, const char *filter,\n"
                    INDENT_TWICE "int *n, %s_t **%ss_array);\n", 
                    table_desc->name, table_desc->columns[i].foreign_table_name,
                    table_desc->name, table_desc->name);
            fprintf(file, "int %s_select_all_list_by_%s(\n"
                    INDENT_TWICE "const sorm_connection_t *conn,\n"
                    INDENT_TWICE "const char *column_names, const char *filter,\n" 
                    INDENT_TWICE "int *n, sorm_list_t **%ss_list_head);\n\n",
                    table_desc->name, table_desc->columns[i].foreign_table_name,
                    table_desc->name);
        }
    }
}

static void header_generate_func_index(
        FILE *file, const sorm_table_descriptor_t *table_desc)
{
    fprintf(file, "int %s_create_index(\n"
            INDENT_TWICE "const sorm_connection_t *conn, char *columns_name);\n",
            table_desc->name);
    fprintf(file, "int %s_drop_index(\n"
            INDENT_TWICE "const sorm_connection_t *conn, char *columns_name);\n",
            table_desc->name);
}

void header_generate(
        const sorm_table_descriptor_t *table_desc)
{
    FILE *file;
    char *file_name, *upper_table_name, *upper_column_name;
    int ret, offset;
    int table_name_len;
    int i;

    table_name_len = strlen(table_desc->name);
    file_name = sys_malloc(table_name_len + 8);
    sprintf(file_name, "%s_sorm.h", table_desc->name);

    file = fopen(file_name, "w");
    sys_free(file_name);
    if(file == NULL)
    {
        error("fopen file(%s) error : %s.", file_name, strerror(errno));
        return;
    }

    upper_table_name = sys_malloc(table_name_len + 1);
    case_lower2upper(table_desc->name, upper_table_name);

    fprintf(file, "#ifndef %s_SORM_H\n"
            "#define %s_SORM_H\n\n", upper_table_name, upper_table_name);
    fprintf(file, "#include \"sorm.h\"\n\n");

    /* #define XX_XX_MAX_LEN XX */
    for(i = 0; i < table_desc->columns_num; i ++)
    {
        upper_column_name = 
            sys_malloc(strlen(table_desc->columns[i].name) + 1);
        case_lower2upper(table_desc->columns[i].name, upper_column_name);
        fprintf(file, "#define %s_%s_MAX_LEN %d\n", 
                upper_table_name, upper_column_name, 
                table_desc->columns[i].max_len);
        free(upper_column_name);
        upper_column_name = NULL;
    }
    fprintf(file, "\n");

    /* #define XXXX_DESC xxxx_get_desc() */
    fprintf(file, "#define %s_DESC %s_get_desc()\n\n", upper_table_name, 
            table_desc->name);

    sys_free(upper_table_name);

    /* typedef struct xxxx_s */
    header_generate_struct(file, table_desc);
    /* functions */
    header_generate_func_get_desc(file, table_desc);
    header_generate_func_new(file, table_desc);
    header_generate_func_free(file, table_desc);
    header_generate_func_create_table(file, table_desc);
    header_generate_func_delete_table(file, table_desc);
    header_generate_func_save(file, table_desc);
    header_generate_func_set_mem(file, table_desc);
    header_generate_func_delete(file, table_desc);
    header_generate_func_select(file, table_desc);
    header_generate_func_index(file, table_desc);

    fprintf(file, "#endif");

}
