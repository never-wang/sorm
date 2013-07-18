%{
#include "parser_yacc.h"
extern int yylval;
%}
%%
"create"            { return CREATE; }
"table"             { return TABLE; }
"{"                 { return LEFT_BRACKET; }
"("                 { return LEFT_DASH; }
"}"                 { return RIGHT_BRACKET; }
")"                 { return RIGHT_DASH; }
","                 { return COMMA; }
"text"              { return TEXT; }
"integer"           { return INTEGER; }
"real"              { return REAL; }
"primary key"       { return PRIMARY_KEY; }
"asc"               { return ASC; }
"desc"              { return DESC; }
"unique"            { return UNIQUE; }
[a-z_]+              { yylval = strdup(yytext); return NAME; }
[0-9]+              { yylval = strdup(yytext); return NUMBER; }
%%

int yywrap()
{
return 1;
}
