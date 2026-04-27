all: compiler

compiler: lex.yy.c parser.tab.c ast.c ir.c
	gcc lex.yy.c parser.tab.c ast.c ir.c -lfl -o compiler

parser.tab.c parser.tab.h: parser.y
	bison -d parser.y

lex.yy.c: lexer.l parser.tab.h
	flex lexer.l

clean:
	rm -f lex.yy.c parser.tab.c parser.tab.h compiler ast.txt