%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

Node* root;
int yylex();
int syntax_error = 0;
int is_void_main = 0;
void yyerror(const char *s);
extern char* yytext;

void process_program(Node* root) {
    if (syntax_error) {
        printf("Compilation stopped due to syntax errors.\n");
        return;
    }
    FILE *f = fopen("ast.txt", "w");
    if (f) {
        fprintf(f, "AST:\n");
        if (root)
            print_ast_file(root, f);
        else
            fprintf(f, "EMPTY AST");
        fprintf(f, "\n");
        fclose(f);
    }

    semantic_error = 0;
    reset_symbol_table();
    semantic_check(root);

    if (semantic_error == 0) {
        generate_3ac(root);
    } else {
        printf("\nCompilation stopped due to semantic errors.\n");
    }
}
%}

%union {
    char* str;
    Node* node;
}

%token <str> ID NUMBER STRING

%token INT MAIN FLOAT DOUBLE VOID CHAR
%token LBRACE RBRACE
%token SEMI
%token RETURN
%token ASSIGN
%token PRINT SCAN
%token WHILE FOR DO

%token LT GT LE GE EQ NE
%token AND OR
%token LPAREN RPAREN
%token INC DEC
%token ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN
%token NOT AND_SINGLE
%token COMMA
%token IF ELSE

%nonassoc IFX
%nonassoc ELSE

%type <node> program stmt stmt_list simple_stmt expr expr_list block decl_list decl_item


%left OR
%left AND
%left AND_SINGLE
%left EQ NE
%left LT GT LE GE
%left '+' '-'
%left '*' '/' '%'
%right NOT
%define parse.error verbose
%%

program:
    stmt_list {
        root = $1;
        process_program(root);
    }
  | INT MAIN LPAREN RPAREN block {
        is_void_main = 0;
        root = $5;
        process_program(root);
    }
  | VOID MAIN LPAREN RPAREN block {
        is_void_main = 1;
        root = $5;
        process_program(root);
    }
;

block:
    LBRACE stmt_list RBRACE   { $$ = create_node("BLOCK", $2, NULL); }
;

decl_list:
      decl_list COMMA decl_item {
          $$ = create_node(";", $1, $3);
      }
    | decl_item {
          $$ = $1;
      }
;

decl_item:
      ID {
          $$ = create_node("DECL", create_leaf($1), NULL);
      }
    | ID ASSIGN expr {
          $$ = create_node("DECL_ASSIGN", create_leaf($1), $3);
      }
;

stmt_list:
      /* empty */      { $$ = NULL; }
    | stmt_list stmt   { if ($1 == NULL) $$ = $2; else $$ = create_node(";", $1, $2); }
;
stmt:
    simple_stmt SEMI                        { $$ = $1; }

    | PRINT LPAREN expr_list RPAREN SEMI
    {
        $$ = create_node("PRINT", $3, NULL);
    }

    | SCAN LPAREN expr_list RPAREN SEMI
    {
        $$ = create_node("SCAN", $3, NULL);
    }

    | IF LPAREN expr RPAREN stmt %prec IFX
    {
        $$ = create_node("IF", $3, $5);
    }

    | IF LPAREN expr RPAREN stmt ELSE stmt
    {
        Node* ifNode = create_node("IF", $3, $5);
        $$ = create_node("IF_ELSE", ifNode, $7);
    }


    | WHILE LPAREN expr RPAREN stmt
    {
        $$ = create_node("WHILE", $3, $5);
    }

    | FOR LPAREN simple_stmt SEMI expr SEMI simple_stmt RPAREN stmt
    {
        $$ = create_node("FOR", $3, create_node("FOR_CTRL", $5, create_node("FOR_BODY", $9, $7)));
    }

    | FOR LPAREN SEMI SEMI RPAREN stmt
    {
        $$ = create_node("WHILE_TRUE", $6, NULL);
    }

    | DO stmt WHILE LPAREN expr RPAREN SEMI
    {
        $$ = create_node("DO_WHILE", $2, $5);
    }

    | RETURN expr SEMI
    {
        $$ = create_node("RETURN", $2, NULL);
    }

    | block
    {
        $$ = $1;
    }

    | SEMI
    {
        $$ = NULL;
    }
;

expr_list:
      expr {
          $$ = create_list($1);
      }
    | expr_list COMMA expr {
          $1->children[$1->child_count++] = $3;
          $$ = $1;
      }
;

simple_stmt:
    INT decl_list {
        $$ = $2;
    }
    | FLOAT decl_list  {
        $$ = $2;
    }
    | DOUBLE decl_list {
        $$ = $2;
    }
    | CHAR decl_list {
        $$ = $2;
    }
    | ID ASSIGN expr {
          $$ = create_node("=", create_leaf($1), $3);
      }

    | ID INC {
          $$ = create_node("=", create_leaf($1),
                create_node("+", create_leaf($1), create_leaf("1")));
      }

    | INC ID {
          $$ = create_node("=", create_leaf($2),
                create_node("+", create_leaf($2), create_leaf("1")));
      }

    | ID DEC {
          $$ = create_node("=", create_leaf($1),
                create_node("-", create_leaf($1), create_leaf("1")));
      }

    | DEC ID {
          $$ = create_node("=", create_leaf($2),
                create_node("-", create_leaf($2), create_leaf("1")));
      }

    | ID ADD_ASSIGN expr {
          $$ = create_node("=", create_leaf($1),
                create_node("+", create_leaf($1), $3));
      }

    | ID SUB_ASSIGN expr {
          $$ = create_node("=", create_leaf($1),
                create_node("-", create_leaf($1), $3));
      }

    | ID MUL_ASSIGN expr {
          $$ = create_node("=", create_leaf($1),
                create_node("*", create_leaf($1), $3));
      }

    | ID DIV_ASSIGN expr {
          $$ = create_node("=", create_leaf($1),
                create_node("/", create_leaf($1), $3));
    }

;


expr:
      expr '+' expr     { $$ = create_node("+", $1, $3); }
    | expr '-' expr     { $$ = create_node("-", $1, $3); }
    | expr '*' expr     { $$ = create_node("*", $1, $3); }
    | expr '/' expr     { $$ = create_node("/", $1, $3); }
    | expr '%' expr     { $$ = create_node("%", $1, $3); }

    | expr LT expr      { $$ = create_node("<", $1, $3); }
    | expr GT expr      { $$ = create_node(">", $1, $3); }
    | expr LE expr      { $$ = create_node("<=", $1, $3); }
    | expr GE expr      { $$ = create_node(">=", $1, $3); }
    | expr EQ expr      { $$ = create_node("==", $1, $3); }
    | expr NE expr      { $$ = create_node("!=", $1, $3); }

    | expr AND expr     { $$ = create_node("&&", $1, $3); }
    | expr OR expr      { $$ = create_node("||", $1, $3); }
    | NOT expr          { $$ = create_node("!", $2,NULL); }
    | AND_SINGLE expr %prec NOT { $$ = create_node("ADDR", $2, NULL); }
    | expr AND_SINGLE expr {$$ = create_node("&", $1, $3);}

    | LPAREN expr RPAREN   { $$ = $2; }
    | ID                { $$ = create_leaf($1); }
    | NUMBER            { $$ = create_leaf($1); }
    | STRING { $$ = create_leaf($1); }
;

%%

void yyerror(const char *s) {
    if (yytext && strlen(yytext) > 0)
        printf("Syntax Error near '%s': %s\n", yytext, s);
    else
        printf("Syntax Error: %s\n", s);
    syntax_error = 1;
}

int main() {
    yyparse();
    return 0;
}
