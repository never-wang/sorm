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
        sorm_list_entry = (sorm_list_t*)_malloc(NULL, sizeof(sorm_list_t)); \
        sorm_list_entry->data = column_desc;    \
        list_add_tail(sorm_list_entry, columns_list_head);  \
        column_desc = (sorm_column_descriptor_t*)_malloc(NULL, sizeof(sorm_column_descriptor_t)); \
        memset(column_desc, 0, sizeof(sorm_column_descriptor_t));   \
        table_desc->columns_num ++; \
    }while(0)
%}

%start ROOT

%token CREATE
%token TABLE
%token LEFT_BRACKET
%token LEFT_DASH
%token RIGHT_BRACKET
%token RIGHT_DASH
%token COMMA
%token INTEGER
%token BIGINT
%token TEXT
%token REAL
%token PRIMARY_KEY
%token ASC
%token DESC 
%token UNIQUE
%token NAME
%token NUMBER
%token FOREIGN_KEY
%token REFERENCES
%token BLOB
%token IGNORE

%%

ROOT :
     statement 
     ;

statement:
         CREATE TABLE ignores NAME LEFT_DASH columns_def COMMA table_defs RIGHT_DASH 
         { 
           table_desc->name = $4; 
         }
	     | CREATE TABLE ignores NAME LEFT_DASH columns_def RIGHT_DASH 
         { 
           table_desc->name = $4; 
         }
         ;

ignores : 
        ignores IGNORE
        | IGNORE

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
            | BIGINT
            { column_desc->type = SORM_TYPE_INT64; }
            | text_type
            { column_desc->type = SORM_TYPE_TEXT; }
            | REAL
            { column_desc->type = SORM_TYPE_DOUBLE; }
	        | blob_type
	        { column_desc->type = SORM_TYPE_BLOB; }
	       ;

text_type :
          TEXT
          { column_desc->mem = SORM_MEM_HEAP; }
          | TEXT LEFT_DASH NUMBER RIGHT_DASH
          { column_desc->mem = SORM_MEM_STACK;
            column_desc->max_len = atoi($3); }
	      ;

blob_type :
          BLOB
          { column_desc->mem = SORM_MEM_HEAP; }
          | BLOB LEFT_DASH NUMBER RIGHT_DASH
          { column_desc->mem = SORM_MEM_STACK;
            column_desc->max_len = atoi($3); }
	      ;

column_constraint :
           use_column_constraint
           | ignores use_column_constraint
           | use_column_constraint ignores
           | ignores use_column_constraint ignores

use_column_constraint :
           PRIMARY_KEY
           { column_desc->constraint = SORM_CONSTRAINT_PK; }
           | PRIMARY_KEY ASC
           { column_desc->constraint = SORM_CONSTRAINT_PK_ASC; }
           | PRIMARY_KEY DESC
           { column_desc->constraint = SORM_CONSTRAINT_PK_DESC; }
           | UNIQUE
           { column_desc->constraint = SORM_CONSTRAINT_UNIQUE; }
           ;

table_defs :
	   table_defs COMMA table_def
	   | table_def
	  
table_def :
	  FOREIGN_KEY LEFT_DASH NAME RIGHT_DASH REFERENCES NAME LEFT_DASH NAME RIGHT_DASH
      { 
          sorm_list_t *pos = NULL;
          sorm_column_descriptor_t *column_desc;
          sorm_list_for_each(pos, columns_list_head)
          {
              column_desc = (sorm_column_descriptor_t *)pos->data;
              if(strcmp(column_desc->name, $3) == 0)
              {
                  column_desc->is_foreign_key = 1;
                  column_desc->foreign_table_name = $6;
                  column_desc->foreign_column_name = $8;
                  break;
              }
          }
      }
      | UNIQUE LEFT_DASH unique_columns RIGHT_DASH

unique_columns :
      unique_columns COMMA NAME |
      NAME
%%

int yyerror(char *msg)
{
printf("Error encountered: %s\n", msg);
return -1;
}


