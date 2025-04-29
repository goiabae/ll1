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

/* pre-declare aliases table functions. */
void aliases_init(void);
void aliases_free(void);
void aliases_add(char* symbol, char* alias);
char* aliased_from(char* name);

/* pre-declare symbol table functions. */
void symbols_init (void);
void symbols_free (void);
int symbols_find (char *name);
int symbols_add (char *name, int is_terminal);
void symbols_print (int is_terminal);
void symbols_print_empty (void);
void symbols_print_first (void);
void symbols_print_follow (void);

/* pre-declare production list functions. */
void prods_init (void);
void prods_free (void);
void prods_add (int lhs, int **rhsv);
void prods_print (void);
void prods_print_predict (void);

/* pre-declare functions to learn information about the grammar. */
void derives_empty (void);
void first (void);
void follow (void);
void predict (void);
bool conflicts (void);

/* pre-declare symbol array functions. */
int symv_len (int *sv);
int *symv_new (int s);
int *symv_add (int *sv, int s);
void symv_print (int *sv);

/* pre-declare symbol double-array functions. */
int symvv_len (int **vv);
int **symvv_new (int *v);
int **symvv_add (int **vv, int *v);

/* aliases table. */
extern struct alias* aliases;
extern int alias_count;

/* symbol table. */
extern struct symbol *symbols;
extern int n_symbols;

/* production list. */
extern struct production *prods;
extern int n_prods;

/* pre-declare variables used by derp(), main(), yyparse() */
extern const char *argv0;

#endif
