/****************************************************************************
 *       Filename:  sorm_generate.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/22/2013 02:37:22 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *          Email:  never.wencan@gmail.com
 ***************************************************************************/
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "parser_yacc.h"
#include "stdio.h"
#include "generate.h"
#include "sorm.h"
#include "memory.h"
#include "c_generate.h"
#include "header_generate.h"

sorm_table_descriptor_t *table_desc = NULL;
sorm_list_t *columns_list_head = NULL;
sorm_column_descriptor_t *column_desc = NULL;

extern FILE *yyin;

int main(int argc, char **argv)
{
    sorm_list_t *pos, *scratch;
    int i, argv_index;
    int has_PK = 0;

    if(argc < 2)
    {
        printf("Usage : sorm_generate input_files, "
                "input files are separated by space\n");
        return -1;
    }

    for(argv_index = 1; argv_index < argc; argv_index ++)
    {
        table_desc = _malloc(NULL, sizeof(sorm_table_descriptor_t));
        memset(table_desc, 0, sizeof(sorm_table_descriptor_t));

        columns_list_head = _malloc(NULL, sizeof(sorm_list_t));
        INIT_LIST_HEAD(columns_list_head);

        column_desc = _malloc(NULL, sizeof(sorm_column_descriptor_t));
        memset(column_desc, 0, sizeof(sorm_column_descriptor_t));

        yyin = fopen(argv[argv_index], "r");

        if(yyin == NULL)
        {
            printf("Input file(%s) open fail : %s\n", 
                    argv[argv_index], strerror(errno));
            continue;
        }

        yyparse();

        _free(NULL, column_desc);
        column_desc = NULL;

        /* read create sql */
        int ret = fseek(yyin, 0, SEEK_SET);
        if (ret != 0) {
            printf("fseek(%s) fail : %s", argv[argv_index],
                    strerror(errno));
        }
        char create_sql[SQL_STMT_MAX_LEN + 1];
        char *ret_str = 
            fgets(create_sql, SQL_STMT_MAX_LEN + 1, yyin);
        if (ret_str == NULL) {
            printf("read create_sql fail\n");
            return -1;
        }
        create_sql[strlen(create_sql) - 1] = '\0';
        table_desc->create_sql = create_sql;
        fclose(yyin);

        table_desc->columns = _malloc(NULL, sizeof(sorm_column_descriptor_t) * 
                table_desc->columns_num);
        i = 0;
        table_desc->PK_index = SORM_NO_PK;
        sorm_list_for_each_safe(pos, scratch, columns_list_head)
        {
            memcpy(&table_desc->columns[i], pos->data, 
                    sizeof(sorm_column_descriptor_t));
            _free(NULL, pos);
            if((table_desc->columns[i].constraint == SORM_CONSTRAINT_PK) ||
                    (table_desc->columns[i].constraint == SORM_CONSTRAINT_PK_ASC) ||
                    (table_desc->columns[i].constraint == SORM_CONSTRAINT_PK_DESC))
            {
                if(has_PK == 0)
                {
                    has_PK = 1;
                    table_desc->PK_index = i;
                }else
                {
                    printf("There should be only one Primary Key in a table.\n");
                    sorm_list_free(NULL, columns_list_head);
                    _free(NULL, table_desc->columns);
                    _free(NULL, table_desc);
                }
            }
            i ++;
        }
        _free(NULL, columns_list_head);

        assert(table_desc != NULL);

        header_generate(table_desc);
        c_generate(table_desc, argv[argv_index]);
        _free(NULL, table_desc->columns);
        _free(NULL, table_desc);
    }

    return 0;
}
