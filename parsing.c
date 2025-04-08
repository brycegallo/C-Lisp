#include "mpc.h"
#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h> // gives us readline()
#include <editline/history.h> // gives us add_history()

long eval_op(long x, char* op, long y) {
	if (strcmp(op, "+") == 0) { return x + y; }
	if (strcmp(op, "-") == 0) { return x - y; }
	if (strcmp(op, "*") == 0) { return x * y; }
	if (strcmp(op, "/") == 0) { return x / y; }
	return 0;
}

long eval(mpc_ast_t* t) {

	//If tagged as a number, return it directy
	if (strstr(t->tag, "number")) {
		return atoi(t->contents);
	}

	// The operator is always a second child
	char* op = t->children[1]->contents;
	
	// we store the third child in 'x'
	long x = eval(t->children[2]);

	int i = 3;
	while (strstr(t->children[i]->tag, "expr")) {
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}

	return x;
}

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
		// use readline to prompt the user for input
		char* input = readline("$ ");
		// record the user's input in history so that they can navigate to previously entered commands with arrow keys
		add_history(input);
		
		/* Parse User Input */
		// initialize new result variable r
		mpc_result_t r;
		// call mpc_parse, passing in the user input, our parser Lispy, and a pointer to the result variable r
		if (mpc_parse("<stdin>", input, Lispy, &r)) {
			// if successful, an internal structure is copied into r's output field
			/* On Success Print the AST */
			mpc_ast_print(r.output); // we can print out the structure with this command
			mpc_ast_delete(r.output); // then we delete it with this command
		} else {
			// if not mpc_parse is not successful, an error is copied into r's error field
			/* Otherwise Print the Error */
			mpc_err_print(r.error); // we print out the error
			mpc_err_delete(r.error); // we delete the error
		}
		// we free up the memory allocated to the user input
		free(input);
	}
	
	/* Clean-up: Undefine and Delete Parsers */
	mpc_cleanup(4, Number, Operator, Expr, Lispy);
	return 0;
}
