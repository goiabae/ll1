#ifndef GRAMMAR_H
#define GRAMMAR_H

#include <stdbool.h>
#include <stdio.h>

typedef struct file_t {
  FILE* descriptor;
  char* name;
} file_t;

/* data structure for holding grammar symbol information.
 */
struct symbol {
  /* symbol @name */
  char *name;

  /* @first and @follow sets for non-terminal symbols. */
  int *first, *follow;
  int visited;

  /* whether the symbol @is_terminal or @derives_empty. */
  int is_terminal;
  int derives_empty;
};

/* data structure for holding productions of the grammar.
 */
struct production {
  /* @lhs: left-hand side one-based symbol table index.
   * @rhs: right-hand side one-based symbol table indices.
   */
  int lhs, *rhs;

  /* terminal @yield of nonterminals and whether
   * a nonterminal @derives_empty.
   */
  int yield;
  int derives_empty;

  /* @predict set for each production. */
  int *predict;
};

struct alias {
  char* to;
  char* from;
};

typedef struct grammar_t {
	/* aliases table. */
	 struct alias* aliases;
	 int alias_count;

	/* symbol table. */
	 struct symbol *symbols;
	 int n_symbols;

	/* production list. */
	 struct production *prods;
	 int n_prods;
} grammar_t;

/* pre-declare aliases table functions. */
void aliases_init(grammar_t* g);
void aliases_free(grammar_t* g);
void aliases_add(grammar_t* g, char* symbol, char* alias);
char* aliased_from(grammar_t* g, char* name);

/* pre-declare symbol table functions. */
void symbols_init (grammar_t* g);
void symbols_free (grammar_t* g);
int symbols_find (grammar_t* g, char *name);
int symbols_add (grammar_t* g, char *name, int is_terminal);
void symbols_print (grammar_t* g, int is_terminal);
void symbols_print_empty (grammar_t* g);
void symbols_print_first (grammar_t* g);
void symbols_print_follow (grammar_t* g);

/* pre-declare production list functions. */
void prods_init (grammar_t* g);
void prods_free (grammar_t* g);
void prods_add (grammar_t* g, int lhs, int **rhsv);
void prods_print (grammar_t* g);
void prods_print_predict (grammar_t* g);

/* pre-declare functions to learn information about the grammar. */
void derives_empty (grammar_t* g);
void first (grammar_t* g);
void follow (grammar_t* g);
void predict (grammar_t* g);
bool conflicts (grammar_t* g);

/* pre-declare symbol array functions. */
int symv_len (int *sv);
int *symv_new (int s);
int *symv_add (int *sv, int s);
void symv_print (grammar_t* g, int *sv);

/* pre-declare symbol double-array functions. */
int symvv_len (int **vv);
int **symvv_new (int *v);
int **symvv_add (int **vv, int *v);

#define STR_EPSILON "%empty"
#define STR_TOKEN "%token"

#endif
