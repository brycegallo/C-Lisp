#include "mpc.h"
#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>
#include <editline/history.h>

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	/* Parsers */
	mpc_parser_t* Number = mpc_new("number");	
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Lispy = mpc_new("lispy");

	/* Language */
	mpca_lang(MPCA_LANG_DEFAULT,
	"							\
	number	: /-?[0-9]+/ ;					\
	operator: '+' | '-' | '*' | '/' ;			\
	expr	: <number> | '(' <operator> <expr>+ ')' ;	\
	lispy	: /^/ <operator> <expr>+ /$/ ;			\
	",
	Number, Operator, Expr, Lispy);

	/* Print Version and Exit Information */
	puts("C-lisp Version 0.0.1");
	puts("Press ctrl-c to Exit\n");

	while (1) {
		char* input = readline("$ ");
		add_history(input);
		printf("%s\n", input);
		free(input);
	}
	
	/* Clean-up: Undefine and Delete Parsers */
	mpc_cleanup(4, Number, Operator, Expr, Lispy);
	return 0;
}
