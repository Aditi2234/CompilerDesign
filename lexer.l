%{
#include "ast.h"  
#include "parser.tab.h"
#include <stdlib.h>
#include <string.h>

%}


%%
"//".*                          {  }
"/*"([^*]|\*+[^*/])*\*+"/"      {  }
"#"[^\n]*       {  }



"int"      { return INT; }
"float"   { return FLOAT; }
"double"   { return DOUBLE; }
"void"     { return VOID; }
"main"     { return MAIN; }
"return"   { return RETURN; }
"printf"   { return PRINT; }
"scanf"    { return SCAN; }
"while"    { return WHILE; }
"for"      { return FOR; }
"do"       { return DO; }


"if"       { return IF; }
"else"     { return ELSE; }
"=="       { return EQ; }
"!="       { return NE; }
"<="       { return LE; }
">="       { return GE; }
"<"        { return LT; }
">"        { return GT; }


"&&"       { return AND; }
"||"       { return OR; }
"!"        { return NOT; }
"&"        { return AND_SINGLE; }

"{"        { return LBRACE; }
"}"        { return RBRACE; }

"("        { return LPAREN; }
")"        { return RPAREN; }

","        { return COMMA; }
";"        { return SEMI; }

"+="    { return ADD_ASSIGN; }
"-="    { return SUB_ASSIGN; }
"*="    { return MUL_ASSIGN; }
"/="    { return DIV_ASSIGN; }
"++"    { return INC; }
"--"    { return DEC; }
"="        { return ASSIGN; }
"+"        { return '+'; }
"-"        { return '-'; }
"*"        { return '*'; }
"/"        { return '/'; }
"%"        { return '%'; }


\"[^\"]*\"    { yylval.str = strdup(yytext);
                return STRING; 
                }
[0-9]+     { yylval.str = strdup(yytext); return NUMBER; }
[0-9]+\.[0-9]+   { yylval.str = strdup(yytext); return NUMBER; }



[a-zA-Z_][a-zA-Z0-9_]* {
    yylval.str = strdup(yytext);
    return ID;
}

[ \t\n]+   ;   // ignore whitespace

.          ;   // ignore unknown
%%

int yywrap() {
    return 1;
}
