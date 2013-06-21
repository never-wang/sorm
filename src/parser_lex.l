%{
#include "y.tab.h"
extern int yylval;
%}
%%
"create"            { return CREATE; }
"table"             { return TABLE; }
"{"                 { return LEFT_BRACKET; }
"}"                 { return RIGHT_BRACKET; }
","                 { return COMMA; }
"text"              { return TEXT; }
"integer"           { return INTEGER; }
"real"              { return REAL; }
"primary key"       { return PRIMARY_KEY; }
"asc"               { return ASC; }
"desc"              { return DESC; }
"unique"            { return UNIQUE; }
[a-z]+              { yylval = strdup(yytext); 
                      return NAME; }
%%

int yywrap()
{
return 1;
}
