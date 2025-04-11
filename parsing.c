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
    // Error and Symbol types have some string data 
    char* err;
    char* sym;
    // Count and Pointer to a list of lval*s
    int count;
    // S-Expressions are variable-length lists of other values. Since we can't create variable-length structs, we're going to create a pointer field "cell" which points to a location where we store a list of of lval*s, which are pointers to other individual lvals. our field should be a double pointer of type lval** (a pointer to pointers). We'll use "count" to keep track of how many lval*s are in the list
    struct lval** cell;
} lval;

/*** enums ***/

// Enumeration of possible S-Expression lval types
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR };

// Enumeration of possible error types
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/*** structs ***/

// We have changed our lval construction functions to return pointers to an lval, rather than return an lval directly, to make keeping track of lvals easier. Now to make this work we need to use malloc with the sizeof() function to allocate enough space for the lval struct, and then fill in the fields with the relevant information using the arrow operator

// Construct a pointer to a new number of type lval
lval* lval_num(long x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

// Construct a pointer to a new error of type lval
lval* lval_err(char* m) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(m) + 1); // we use strlen() + 1 because strings in C are null-terminated, always having their final character as the zero character '\0'. it's crucial to have all strings stored this way in C. strlen() only returns the number of bytes in a string excluding the null-terminator, which is why we need to add one when memory-allocating space for a string
    strcpy(v->err, m);
    return v;
}

// Construct a pointer to a new Symbol of type lval
lval* lval_sym(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}

// Construct a pointer to a new empty Sexpr lval
lval* lval_sexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL; // here NULL specifies that we have a data pointer that doesn't point to any number of data items
    return v;
}

// This function will delete an lval*, first inspecting the type, then releasing any memory pointed to by its fields by calling free(). it's important to match every call to one of the above functions with lval_del to ensure we have no memory leaks
void lval_del(lval* v) {
    switch(v->tpe) {
	// Number type lvals need no special free() calls
	case LVAL_NUM: break;
	
	// We must free the string data for Error and Symbol types
	case LVAL_ERR: free(v->err); break;
	case LVAL_SYM: free(v->sym); break;

	// We must delete all the elements inside an S-Expression type
	case LVAL_SEXPR:
		       // We iterate over the Cell and call lval_del() on each element
		       for (int i = 0; i < v->count; i++) {
			   lval_del(v->cell[i]);
		       }
		       // We also free the memory allocated to contain the pointers
		       free(v->cell);
	break;
    }
    // Lastly, we free the memory allocated for the lval struct itself
    free(v);
}

/*** lval functions ***/

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
    (void)argc; // avoid warnings about unused parameters
    (void)argv; // avoid warnings about unused parameters
    
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
