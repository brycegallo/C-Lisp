#include "mpc.h"
#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h> // gives us readline()
#include <editline/history.h> // gives us add_history()

// Declare new lval struct
// In our program any expression will evaluate to either a number or an error, so we want to define a data structure that can be either, to make it easier to return the result to the user
typedef struct {
    int type; // will be represented by a single integer for simple matching
    long num;
    int err;
} lval;

// Enumeration of possible lval types
enum { LVAL_NUM, LVAL_ERR };

// Enumeration of possible error types
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

// Create a new number of type lval
lval lval_num(long x) {
    lval v;
    v.type = LVAL_NUM;
    v.num = x;
    return v;
}

// Create a new error of type lval
lval lval_err(int x) {
    lval v;
    v.type = LVAL_ERR;
    v.err = x;
    return v;
}

// Print an lval by using a switch statement to determine the printing behavior based on whether the lval being printed has a type of number or error
void lval_print(lval v) {
    switch(v.type) {
	// if the type is a number, print it and break out of the switch
	case LVAL_NUM: printf("%li", v.num); break;
	// if the type is an error, check which type it is and print response
	case LVAL_ERR:
		       if (v.err == LERR_DIV_ZERO) { printf("Error: Division by Zero"); }
		       if (v.err == LERR_BAD_OP) { printf("Error: Invalid Operator"); }
		       if (v.err == LERR_BAD_NUM) { printf("Error: Invalid Number"); }
	break;
    }
}

// print an lval followed by a newline
void lval_println(lval v) {
    lval_print(v);
    putchar('\n');
}

// This function will use strcmp to check which operator to use
lval eval_op(lval x, char* op, lval y) {
    // If either operand's type value is an error, return it immediately
    if (x.type == LVAL_ERR) { return x; }
    if (y.type == LVAL_ERR) { return y; }

    // Otherwise, operate on the operands
    if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
    if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
    if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
    if (strcmp(op, "/") == 0) {
	// Return error if second operand is zero
	return y.num == 0
	    ? lval_err(LERR_DIV_ZERO)
	    : lval_num(x.num / y.num);
    }
    return lval_err(LERR_BAD_OP);
}

// This function will use strstr to check if a tag contains some substring
lval eval(mpc_ast_t* t) {
    // If tagged as a number, return it directly, also check for errors in conversion
    if (strstr(t->tag, "number")) {
	errno = 0;
	long x = strtol(t->contents, NULL, 10);
	return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }
    
    // The operator is always a second child
    char* op = t->children[1]->contents;
    // we store the third child in 'x'
    lval x = eval(t->children[2]);
    
    // Iterate the remaining children and combine
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
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Sexpr = mpc_new("sexpr");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Lispy = mpc_new("lispy");
    
    /* Language */
    mpca_lang(MPCA_LANG_DEFAULT,
	    "								\
	    number	: /-?[0-9]+/ ;					\
	    symbol	: '+' | '-' | '*' | '/' ;			\
	    sexpr	: '(' <expr>* ')' ;				\
	    expr	: <number> |  <symbol> <sexpr> ;		\
	    lispy	: /^/ <expr>* /$/ ;			\
	    ",
	    Number, Symbol, Sexpr, Expr, Lispy);
    
    /* Print Version and Exit Information */
    puts("C-lisp Version 0.0.1");
    puts("Press ctrl-c to Exit\n");
    
    while (1) {
	// use readline to prompt the user for input */
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
	    // mpc_ast_print(r.output); // we can print out the structure with this command
	    lval result = eval(r.output);
	    lval_println(result);
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
    mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);
    return 0;
}
