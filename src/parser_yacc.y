%{

#include <stdio.h>
#include <string.h>
#define YYSTYPE char*

#include "generate.h"
#include "sorm.h"
#include "memory.h"

static sorm_list_t *sorm_list_entry = NULL;

#define parse_column(column_name)    \
    do{ \
        column_desc->name = (column_name);   \
        column_desc->mem = SORM_MEM_HEAP;  \
        sorm_list_entry = (sorm_list_t*)mem_malloc(sizeof(sorm_list_t)); \
        sorm_list_entry->data = column_desc;    \
        list_add_head(sorm_list_entry, columns_list_head);  \
        column_desc = (sorm_column_descriptor_t*)mem_malloc(sizeof(sorm_column_descriptor_t)); \
        memset(column_desc, 0, sizeof(sorm_column_descriptor_t));   \
        table_desc->columns_num ++; \
    }while(0)
%}

%start ROOT

%token CREATE
%token TABLE
%token LEFT_BRACKET
%token RIGHT_BRACKET
%token COMMA
%token INTEGER
%token TEXT
%token REAL
%token PRIMARY_KEY
%token ASC
%token DESC 
%token UNIQUE
%token NAME

%%

ROOT :
     statement
     ;

statement:
         CREATE TABLE NAME LEFT_BRACKET columns_def RIGHT_BRACKET
         { 
           //printf("CREATE TABLE %s { %s } \n", $3, $5); 
           table_desc->name = $3; 
         }
         ;

columns_def :
            columns_def COMMA column_def
           | column_def
           ;

column_def :
           NAME column_type
           { parse_column($1); }
           | NAME column_type column_constraint
           { parse_column($1); }
           ;

column_type :
            INTEGER
           { column_desc->type = SORM_TYPE_INT; }
            | TEXT
           { column_desc->type = SORM_TYPE_TEXT; }
            | REAL
           { column_desc->type = SORM_TYPE_DOUBLE; }


column_constraint :
                  PRIMARY_KEY
           { column_desc->constraint = SORM_CONSTRAINT_PK; }
           | PRIMARY_KEY ASC
           { column_desc->constraint = SORM_CONSTRAINT_PK_ASC; }
           | PRIMARY_KEY DESC
           { column_desc->constraint = SORM_CONSTRAINT_PK_DESC; }
           | UNIQUE
           { column_desc->constraint = SORM_CONSTRAINT_UNIQUE; }
           ;
%%

int yyerror(char *msg)
{
printf("Error encountered: %s\n", msg);
return -1;
}


