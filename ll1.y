
/* ll1.y: a dirty hack for checking if a bison-format CFG is LL(1).
 *
 * Copyright (c) 2016 Bradley Worley <geekysuavo@gmail.com>
 * Released under the MIT License. See LICENSE.md for details.
 */

/* enable verbose errors and debugging information in bison. */
%define parse.error verbose
%debug

%define api.pure full
%locations

%lex-param {file_t file}
%parse-param {file_t file}

%code requires {
  #include "grammar.h"
}

%{

/* include the (bison-generated) main header file. */
#include "ll1.h"
#include "grammar.h"

/* pre-declare functions used by yyparse(). */
void yyerror (YYLTYPE* yylloc, file_t file, const char *msg);
int yylex (YYSTYPE* yylval, YYLTYPE* yylloc, file_t file);
%}

/* define the data structure used for passing attributes with symbols
 * in the parsed grammar.
 */
%union {
  /* @sym: one-based symbol table index.
   * @symv: zero-terminated array of symbol table indices.
   * @symvv: null-terminated array of symbol table index arrays.
   * @id: identifier string prior to symbol table translation.
   */
  int sym, *symv, **symvv;
  char *id;
}

/* define the set of terminal symbols to parse. */
%token TOKEN EPSILON
%token ID DERIVES END OR ALIAS

/* set up attribute types of nonterminals. */
%type<sym> symbol
%type<symv> symbols
%type<symvv> productions

/* set up attribute types of terminals. */
%type<id> ID EPSILON ALIAS

%%

grammar
  : opt_preamble rules
  ;

opt_preamble
  : opt_directive_list
  ;

opt_directive_list
  : %empty
  | directive_list
  ;

directive_list
  : directive opt_directive_list
  ;

directive
  : TOKEN ID ALIAS
  { aliases_add($ID, $ALIAS); }
  | TOKEN ID
  ;

rules : rules rule | rule ;

rule : ID DERIVES productions END { prods_add(symbols_add($1, 0), $3); };

productions : productions OR symbols { $$ = symvv_add($1, $3); }
            | symbols                { $$ = symvv_new($1);     };

symbols : symbols symbol { $$ = symv_add($1, $2); }
        | symbol         { $$ = symv_new($1);     };

symbol : ID      { $$ = symbols_add($1, 1); }
       | EPSILON { $$ = symbols_add($1, 1); }
       | ALIAS   { $$ = symbols_add(aliased_from($1), 1); };
