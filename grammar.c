#define _POSIX_C_SOURCE 200809L

#include "grammar.h"
#include "main.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

void aliases_init(grammar_t* g) {
  g->aliases = malloc(sizeof(struct alias)*1);
  g->alias_count = 0;
}

void aliases_free(grammar_t* g) {
  free(g->aliases);
}

void aliases_add(grammar_t* g, char* symbol, char* alias) {
  g->aliases = realloc(g->aliases, sizeof(struct alias)*(g->alias_count+1));
  g->aliases[g->alias_count].from = symbol;
  g->aliases[g->alias_count].to = alias;
  g->alias_count++;
}

/* aliased_from(): returns a reference pointer to the string name was aliased to.
 * If name isn't an alias to another symbol, return name as is.
 */
char* aliased_from(grammar_t* g, char* name) {
  for (int i = 0; i < g->alias_count; i++) {
    if (strcmp(g->aliases[i].to, name) == 0)
      return strdup(g->aliases[i].from);
  }
  return name;
}

/* symbol_is_empty(): return whether a symbol (specified by the one-based
 * index @sym) is the epsilon terminal.
 */
int symbol_is_empty (grammar_t* g, int sym) {
  return (sym >= 1 && sym <= g->n_symbols &&
          strcmp(g->symbols[sym - 1].name, STR_EPSILON) == 0);
}

/* symbols_init(): initialize the global symbol table.
 */
void symbols_init (grammar_t* g) {
  g->symbols = NULL;
  g->n_symbols = 0;
}

/* symbols_free(): deallocate the global symbol table.
 */
void symbols_free (grammar_t* g) {
  for (int i = 0; i < g->n_symbols; i++) {
    free(g->symbols[i].name);

    if (g->symbols[i].first)
      free(g->symbols[i].first);

    if (g->symbols[i].follow)
      free(g->symbols[i].follow);
  }

  free(g->symbols);
}

/* symbols_find(): get the one-based index of a symbol (by @name) in the
 * symbol table, or 0 if no such symbol exists.
 */
int symbols_find (grammar_t* g, char *name) {
  int i;

  for (i = 0; i < g->n_symbols; i++) {
    if (strcmp(g->symbols[i].name, name) == 0)
      return i + 1;
  }

  return 0;
}

/* symbols_add(): ensure that a symbol having @name and @is_terminal
 * flag exists in the symbol table. if the symbol @name exists, its
 * @is_terminal flag is updated based on the passed value. the
 * one-based symbol table index is returned.
 */
int symbols_add (grammar_t* g, char *name, int is_terminal) {
  int sym = symbols_find(g, name);
  if (sym) {
    g->symbols[sym - 1].is_terminal &= is_terminal;

    free(name);
    return sym;
  }

  g->symbols = (struct symbol*)
    realloc(g->symbols, ++g->n_symbols * sizeof(struct symbol));

  if (!g->symbols)
    derp("unable to resize symbol table");

  g->symbols[g->n_symbols - 1].name = strdup(name);
  g->symbols[g->n_symbols - 1].is_terminal = is_terminal;
  g->symbols[g->n_symbols - 1].derives_empty = 0;
  g->symbols[g->n_symbols - 1].visited = 0;
  g->symbols[g->n_symbols - 1].first = NULL;
  g->symbols[g->n_symbols - 1].follow = NULL;

  free(name);
  return g->n_symbols;
}

/* symbols_print(): print all symbols in the table with @is_terminal
 * flag equaling a certain value.
 */
void symbols_print (grammar_t* g, int is_terminal) {
  for (int i = 0; i < g->n_symbols; i++) {
    if (g->symbols[i].is_terminal == is_terminal) {
      printf("  %s", g->symbols[i].name);
      for (int j = 0; j < g->alias_count; j++) {
        if (strcmp(g->aliases[j].from, g->symbols[i].name) == 0)
          printf(" (aliases to \"%s\")", g->aliases[j].to);
      }
      printf("\n");
    }
  }
}

/* symbols_print_empty(): print all symbols that may derive epsilon
 * in zero or more steps.
 */
void symbols_print_empty (grammar_t* g) {
  unsigned int len;
  char buf[32];
  int i;

  for (i = 0, len = 0; i < g->n_symbols; i++) {
    if (strlen(g->symbols[i].name) > len)
      len = strlen(g->symbols[i].name);
  }

  snprintf(buf, 32, "  %%%us -->* %%%%empty\n", len);

  for (i = 0; i < g->n_symbols; i++) {
    if (symbol_is_empty(g, i + 1))
      continue;

    if (g->symbols[i].derives_empty)
      printf(buf, g->symbols[i].name);
  }
}

/* symbols_print_first(): print all symbols in the @first sets of all
 * nonterminals.
 */
void symbols_print_first (grammar_t* g) {
  for (int i = 0; i < g->n_symbols; i++) {
    if (g->symbols[i].is_terminal ||
        symv_len(g->symbols[i].first) == 0)
      continue;

    printf("  first(%s):", g->symbols[i].name);
    symv_print(g, g->symbols[i].first);
  }
}

/* symbols_print_follow(): print all symbols in the @follow sets of all
 * nonterminals.
 */
void symbols_print_follow (grammar_t* g) {
  for (int i = 0; i < g->n_symbols; i++) {
    if (g->symbols[i].is_terminal ||
        symv_len(g->symbols[i].follow) == 0)
      continue;

    printf("  follow(%s):", g->symbols[i].name);
    symv_print(g, g->symbols[i].follow);
  }
}

/* symbols_reset_visite(): reset the @visited flag of all symbols
 * to zero. used internally by @first and @follow set construction.
 */
void symbols_reset_visited (grammar_t* g) {
  for (int i = 0; i < g->n_symbols; i++)
    g->symbols[i].visited = 0;
}

/* prods_init(): initialize the global productions list.
 */
void prods_init (grammar_t* g) {
  g->prods = NULL;
  g->n_prods = 0;
}

/* prods_free(): deallocate the global productions list.
 */
void prods_free (grammar_t* g) {
  for (int i = 0; i < g->n_prods; i++) {
    free(g->prods[i].rhs);

    if (g->prods[i].predict)
      free(g->prods[i].predict);
  }

  free(g->prods);
}

/* prods_add(): add a set of productions with left-hand-side symbol index
 * @lhs and right-hand-side symbol index arrays @rhsv to the global
 * productions list.
 */
void prods_add (grammar_t* g, int lhs, int **rhsv) {
  int n = symvv_len(rhsv);

  for (int i = 0; i < n; i++) {
    int *rhs = rhsv[i];

    g->prods = (struct production*)
      realloc(g->prods, ++g->n_prods * sizeof(struct production));

    if (!g->prods)
      derp("unable to resize production list");

    g->prods[g->n_prods - 1].lhs = lhs;
    g->prods[g->n_prods - 1].rhs = rhs;

    g->prods[g->n_prods - 1].yield = 0;
    g->prods[g->n_prods - 1].derives_empty = 0;
    g->prods[g->n_prods - 1].predict = NULL;
  }

  free(rhsv);
}

/* prods_print(): print the global productions list in a format that
 * resembles the original bison grammar.
 */
void prods_print (grammar_t* g) {
  int lhs_prev = 0;

  for (int i = 0; i < g->n_prods; i++) {
    int lhs = g->prods[i].lhs;
    int *rhs = g->prods[i].rhs;

    if (lhs != lhs_prev) {
      printf("\n  %s :", g->symbols[lhs - 1].name);
      lhs_prev = lhs;
    }
    else {
      for (unsigned int j = 0; j < strlen(g->symbols[lhs - 1].name) + 3; j++)
        printf(" ");

      printf("|");
    }

    for (int j = 0; j < symv_len(rhs); j++)
      printf(" %s", g->symbols[rhs[j] - 1].name);

    printf("\n");
  }
}

/* prods_print_predict(): print the @predict sets of all productions.
 */
void prods_print_predict (grammar_t* g) {
  for (int i = 0; i < g->n_prods; i++) {
    int lhs = g->prods[i].lhs;
    int *rhs = g->prods[i].rhs;

    printf("  %s :", g->symbols[lhs - 1].name);
    for (int j = 0; j < symv_len(rhs); j++)
      printf(" %s", g->symbols[rhs[j] - 1].name);

    symv_print(g, g->prods[i].predict);
  }
}

/* symv_len(): get the length of a symbol array. symbols are one-based, so
 * a zero-terminator is used to mark the end of the array.
 */
int symv_len (int *sv) {
  if (!sv)
    return 0;

  int n = 0;
  while (sv[n])
    n++;

  return n;
}

/* symv_new(): construct a new symbol array from a single symbol.
 */
int *symv_new (int s) {
  int *sv = (int*) malloc(2 * sizeof(int));
  if (!sv)
    return NULL;

  sv[0] = s;
  sv[1] = 0;

  return sv;
}

/* symv_add(): create a new symbol array that contains both @sv and @s,
 * free @sv, and return the new array.
 */
int *symv_add (int *sv, int s) {
  if (!sv)
    return symv_new(s);

  int nv = symv_len(sv);
  int *snew = (int*) malloc((nv + 2) * sizeof(int));
  if (!snew) {
    free(sv);
    return NULL;
  }

  memcpy(snew, sv, nv * sizeof(int));

  snew[nv] = s;
  snew[nv + 1] = 0;

  free(sv);
  return snew;
}

/* symv_incl(): create a new array as in symv_add(), but do not add
 * duplicate symbols to the array.
 */
int *symv_incl (int *sv, int s) {
  if (!sv)
    return symv_new(s);

  for (int i = 0; i < symv_len(sv); i++) {
    if (sv[i] == s)
      return sv;
  }

  return symv_add(sv, s);
}

/* symv_intersect(): create a new array that is the intersection of the
 * sets (symbol arrays) @sva and @svb.
 */
int *symv_intersect (int *sva, int *svb) {
  int ia, ib, na, nb, *result;

  result = NULL;
  na = symv_len(sva);
  nb = symv_len(svb);

  for (ia = 0; ia < na; ia++) {
    for (ib = 0; ib < nb; ib++) {
      if (svb[ib] == sva[ia])
        result = symv_incl(result, sva[ia]);
    }
  }

  return result;
}

/* symv_print(): print the symbols (as strings) within a symbol array,
 * making sure to keep pretty pretty formatting.
 */
void symv_print (grammar_t* g, int *sv) {
  unsigned int len;
  int i, n, wrap;
  char buf[16];

  n = symv_len(sv);

  for (i = len = 0; i < n; i++) {
    if (strlen(g->symbols[sv[i] - 1].name) > len)
      len = strlen(g->symbols[sv[i] - 1].name);
  }

  len += 2;
  wrap = 76 / len;
  snprintf(buf, 16, "%%-%us", len);

  printf("\n    ");
  for (i = 0; i < n; i++) {
    printf(buf, g->symbols[sv[i] - 1].name);

    if ((i + 1) % wrap == 0 && i < n - 1)
      printf("\n    ");
  }

  printf("\n\n");
}

/* symvv_len(): get the length of a symbol double-array. null-terminators
 * are used to mark the end of the outer array.
 */
int symvv_len (int **vv) {
  if (!vv)
    return 0;

  int n = 0;
  while (vv[n])
    n++;

  return n;
}

/* symvv_new(): construct a new symbol double-array from a single symbol
 * array.
 */
int **symvv_new (int *v) {
  int **vv = (int**) malloc(2 * sizeof(int*));
  if (!vv)
    return NULL;

  vv[0] = v;
  vv[1] = NULL;

  return vv;
}

/* symvv_add(): create a new symbol double-array that contains both @vv
 * and @v, free @vv, and return the new double-array.
 */
int **symvv_add (int **vv, int *v) {
  int nv = symvv_len(vv);
  int **vnew = (int**) malloc((nv + 2) * sizeof(int*));
  if (!vnew) {
    free(vv);
    return NULL;
  }

  memcpy(vnew, vv, nv * sizeof(int*));

  vnew[nv] = v;
  vnew[nv + 1] = 0;

  free(vv);
  return vnew;
}

/* derives_empty_check_prod(): internal worker function for derives_empty().
 */
void derives_empty_check_prod (grammar_t* g, int i, int **work) {
  if (g->prods[i].yield == 0) {
    g->prods[i].derives_empty = 1;

    if (g->symbols[g->prods[i].lhs - 1].derives_empty == 0) {
      g->symbols[g->prods[i].lhs - 1].derives_empty = 1;
      *work = symv_add(*work, g->prods[i].lhs);
    }
  }
}

/* derives_empty(): determine which symbols and productions in the grammar
 * are capable of deriving epsilon in any number of steps.
 */
void derives_empty (grammar_t* g) {
  int i, j, k;

  int *work = NULL;
  int n_work = 0;

  for (i = 0; i < g->n_symbols; i++) {
    if (symbol_is_empty(g, i + 1))
      g->symbols[i].derives_empty = 1;
    else
      g->symbols[i].derives_empty = 0;
  }

  for (i = 0; i < g->n_prods; i++) {
    g->prods[i].yield = 0;
    g->prods[i].derives_empty = 0;

    for (j = 0; j < symv_len(g->prods[i].rhs); j++) {
      if (!symbol_is_empty(g, g->prods[i].rhs[j]))
        g->prods[i].yield++;
    }

    derives_empty_check_prod(g, i, &work);
  }

  n_work = symv_len(work);
  while (n_work) {
    k = work[0];
    work[0] = work[n_work - 1];
    work[n_work - 1] = 0;

    for (i = 0; i < g->n_prods; i++) {
      for (j = 0; j < symv_len(g->prods[i].rhs); j++) {
        if (g->prods[i].rhs[j] != k)
          continue;

        g->prods[i].yield--;
        derives_empty_check_prod(g, i, &work);
      }
    }

    n_work = symv_len(work);
  }

  free(work);
}

/* first_set(): determine the @first set of a given set of symbols.
 */
int *first_set (grammar_t* g, int *set) {
  int i, j, *result, *fi_rhs;

  if (symv_len(set) == 0)
    return symv_new(0);

  if (g->symbols[set[0] - 1].is_terminal)
    return symv_new(set[0]);

  result = NULL;

  if (g->symbols[set[0] - 1].visited == 0) {
    g->symbols[set[0] - 1].visited = 1;

    for (i = 0; i < g->n_prods; i++) {
      if (g->prods[i].lhs != set[0])
        continue;

      fi_rhs = first_set(g, g->prods[i].rhs);
      for (j = 0; j < symv_len(fi_rhs); j++)
        result = symv_incl(result, fi_rhs[j]);

      free(fi_rhs);
    }
  }

  if (g->symbols[set[0] - 1].derives_empty) {
    fi_rhs = first_set(g, set + 1);
    for (j = 0; j < symv_len(fi_rhs); j++)
      result = symv_incl(result, fi_rhs[j]);

    free(fi_rhs);
  }

  return result;
}

/* first(): compute the @first sets of all symbols in the grammar.
 */
void first (grammar_t* g) {
  for (int i = 0; i < g->n_symbols; i++) {
    symbols_reset_visited(g);

    int *set = symv_new(i + 1);
    g->symbols[i].first = first_set(g, set);
    free(set);
  }
}

/* follow_set_allempty(): worker function for follow_set().
 */
int follow_set_allempty (grammar_t* g, int *set) {
  for (int i = 0; i < symv_len(set); i++) {
    if (g->symbols[set[i] - 1].derives_empty == 0 ||
        g->symbols[set[i] - 1].is_terminal)
      return 0;
  }

  return 1;
}

/* follow_set(): determine the @follow set of a given nonterminal.
 */
int *follow_set (grammar_t* g, int sym) {
  int *result = NULL;

  if (g->symbols[sym - 1].visited == 0) {
    g->symbols[sym - 1].visited = 1;

    for (int i = 0; i < g->n_prods; i++) {
      for (int j = 0; j < symv_len(g->prods[i].rhs); j++) {
        if (g->prods[i].rhs[j] != sym)
          continue;

        int *tail = g->prods[i].rhs + (j + 1);

        if (*tail) {
          int *fi = g->symbols[*tail - 1].first;
          for (int k = 0; k < symv_len(fi); k++)
            result = symv_incl(result, fi[k]);
        }

        if (follow_set_allempty(g, tail)) {
          int *fo = follow_set(g, g->prods[i].lhs);
          for (int k = 0; k < symv_len(fo); k++)
            result = symv_incl(result, fo[k]);

          free(fo);
        }
      }
    }
  }

  return result;
}

/* follow(): compute the @follow sets of all symbols in the grammar.
 */
void follow (grammar_t* g) {
  for (int i = 0; i < g->n_symbols; i++) {
    symbols_reset_visited(g);

    if (g->symbols[i].is_terminal)
      continue;

    g->symbols[i].follow = follow_set(g, i + 1);

    int *fo = g->symbols[i].follow;
    int nfo = symv_len(fo);

    for (int j = 0; j < nfo; j++) {
      if (symbol_is_empty(g, fo[j])) {
        fo[j] = fo[nfo - 1];
        fo[nfo - 1] = 0;
        break;
      }
    }
  }
}

/* predict_set(): determine the predict set of a given production.
 */
int *predict_set (grammar_t* g, int iprod, int *set) {
  symbols_reset_visited(g);
  int *result = first_set(g ,set);

  if (g->prods[iprod].derives_empty) {
    symbols_reset_visited(g);
    int *fo = follow_set(g, g->prods[iprod].lhs);

    for (int i = 0; i < symv_len(fo); i++)
      result = symv_incl(result, fo[i]);

    free(fo);
  }

  return result;
}

/* predict(): compute the @predict sets of all productions in the grammar.
 */
void predict (grammar_t* g) {
  for (int i = 0; i < g->n_symbols; i++) {
    int lhs = i + 1;

    if (g->symbols[i].is_terminal)
      continue;

    for (int j = 0; j < g->n_prods; j++) {
      if (g->prods[j].lhs != lhs)
        continue;

      g->prods[j].predict = predict_set(g, j, g->prods[j].rhs);

      int *pred = g->prods[j].predict;
      int npred = symv_len(pred);

      for (int k = 0; k < npred; k++) {
        if (symbol_is_empty(g, pred[k])) {
          pred[k] = pred[npred - 1];
          pred[npred - 1] = 0;
          break;
        }
      }
    }
  }
}

/* conflicts_print(): print information about a predict set overlap
 * for a single pair of productions indexed by @id1 and @id2.
 */
void conflicts_print (grammar_t* g, int id1, int id2, int *overlap) {
  printf("  %s :", g->symbols[g->prods[id1].lhs - 1].name);
  for (int i = 0; i < symv_len(g->prods[id1].rhs); i++)
    printf(" %s", g->symbols[g->prods[id1].rhs[i] - 1].name);

  printf("\n  %s :", g->symbols[g->prods[id2].lhs - 1].name);
  for (int i = 0; i < symv_len(g->prods[id2].rhs); i++)
    printf(" %s", g->symbols[g->prods[id2].rhs[i] - 1].name);

  symv_print(g, overlap);
}

/* conflicts(): print all LL(1) conflicts in a grammar, if any.
 */
bool conflicts (grammar_t* g) {
  bool header = false;

  for (int i = 0; i < g->n_symbols; i++) {
    if (g->symbols[i].is_terminal)
      continue;

    for (int j1 = 0; j1 < g->n_prods; j1++) {
      int *pred1 = g->prods[j1].predict;
      if (g->prods[j1].lhs != i + 1)
        continue;

      for (int j2 = j1 + 1; j2 < g->n_prods; j2++) {
        int *pred2 = g->prods[j2].predict;
        if (g->prods[j2].lhs != i + 1)
          continue;

        int *u = symv_intersect(pred1, pred2);

        if (symv_len(u)) {
          if (!header) {
            printf("Conflicts:\n\n");
            header = true;
          }

          conflicts_print(g, j1, j2, u);
        }

        free(u);
      }
    }
  }

  if (header)
    printf("There were conflicts.\nGrammar is not LL(1)\n  :(\n\n");
  else
    printf("No conflicts, grammar is LL(1)\n  :D :D :D\n\n");
  return header;
}
