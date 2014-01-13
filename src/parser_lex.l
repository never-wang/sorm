%{
#include "parser_yacc.h"
extern int yylval;
%}
%%
"CREATE"            { return CREATE; }
"TABLE"             { return TABLE; }
"{"                 { return LEFT_BRACKET; }
"("                 { return LEFT_DASH; }
"}"                 { return RIGHT_BRACKET; }
")"                 { return RIGHT_DASH; }
","                 { return COMMA; }
"TEXT"              { return TEXT; }
"INTEGER"           { return INTEGER; }
"BIGINT"            { return BIGINT; }
"REAL"              { return REAL; }
"PRIMARY KEY"       { return PRIMARY_KEY; }
"ASC"               { return ASC; }
"DESC"              { return DESC; }
"UNIQUE"            { return UNIQUE; }
"FOREIGN KEY"	    { return FOREIGN_KEY; }
"REFERENCES"        { return REFERENCES; }
"BLOB"		    { return BLOB; }
[0-9]+              { yylval = (int)strdup(yytext); return NUMBER; }
[a-z_0-9]+              { yylval = (int)strdup(yytext); return NAME; }
[A-Z]+                  { return IGNORE; }
%%

int yywrap()
{
return 1;
}
