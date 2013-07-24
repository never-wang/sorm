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
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#include <string.h>
#include <errno.h>

#include "parser_yacc.h"
#include "stdio.h"
#include "generate.h"
#include "memory.h"

sorm_table_descriptor_t *table_desc = NULL;
sorm_list_t *columns_list_head = NULL;
sorm_column_descriptor_t *column_desc = NULL;

extern FILE *yyin;

int main(int argc, char **argv)
{
    sorm_list_t *pos, *scratch;
    int i, argv_index;

    if(argc < 2)
    {
        printf("Usage : sorm_generate input_files, input files are separated by space\n");
	return -1;
    }

    for(argv_index = 1; argv_index < argc; argv_index ++)
    {
	table_desc = mem_malloc(sizeof(sorm_table_descriptor_t));
	memset(table_desc, 0, sizeof(sorm_table_descriptor_t));

	columns_list_head = mem_malloc(sizeof(sorm_list_t));
	INIT_LIST_HEAD(columns_list_head);

	column_desc = mem_malloc(sizeof(sorm_column_descriptor_t));
	memset(column_desc, 0, sizeof(sorm_column_descriptor_t));

	yyin = fopen(argv[argv_index], "r");

	if(yyin == NULL)
	{
	    printf("Input file(%s) open fail : %s\n", argv[argv_index], strerror(errno));
	    continue;
	}

	yyparse();
	
	mem_free(column_desc);
	column_desc = NULL;

	table_desc->columns = malloc(sizeof(sorm_column_descriptor_t) * 
		table_desc->columns_num);
	i = 0;
	sorm_list_for_each_safe(pos, scratch, columns_list_head)
	{
	    memcpy(&table_desc->columns[i], pos->data, 
		    sizeof(sorm_column_descriptor_t));
	    mem_free(pos);
	    i ++;
	}
	mem_free(columns_list_head);

	header_generate(table_desc);
	c_generate(table_desc);
	mem_free(table_desc);
    }
    
    return 0;
}
