#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "grammar.h"
#include "ll1.h"
#include "main.h"

const char *argv0 = NULL;

/* yyerror(): error reporting function called by bison on parse errors.
 */
void yyerror (YYLTYPE* yylloc, file_t file, const char *msg) {
  fprintf(stderr, "%s: error: %s:%d: %s\n", argv0, file.name, yylloc->first_line, msg);
}

/* yylex(): lexical analysis function that breaks the input grammar file
 * into a stream of tokens for the bison parser.
 */
int yylex (YYSTYPE* yylval, YYLTYPE* yylloc, file_t file) {
  int c, cprev, ntext;
  char *text;

  while (1) {
    c = fgetc(file.descriptor);
    text = NULL;
    ntext = 0;

    switch (c) {
      case EOF: return c;

      case ':': return DERIVES;
      case ';': return END;
      case '|': return OR;

      case '\n':
        yylloc->first_line++;
        break;
    }

    if (c == '/') {
      c = fgetc(file.descriptor);

      if (c == '/') {
        while (c && c != '\n')
          c = fgetc(file.descriptor);

        if (c == EOF) return c;
        yylloc->first_line++;
      }
      else if (c == '*') {
        cprev = c;
        c = fgetc(file.descriptor);

        while (c && (cprev != '*' || c != '/')) {
          cprev = c;
          c = fgetc(file.descriptor);

          if (c == '\n') yylloc->first_line++;
        }

        if (c == EOF) return c;
      }
      else
        fseek(file.descriptor, -1, SEEK_CUR);
    }

    if (c == '\'') {
      text = (char*) malloc((++ntext + 1) * sizeof(char));
      if (!text)
        derp("unable to allocate token buffer");

      text[0] = fgetc(file.descriptor);
      text[1] = '\0';

      c = fgetc(file.descriptor);
      yylval->id = text;
      if (c == '\'' || text[0] != '\'')
        return ID;

      free(text);
    }

    if (c == '\"') {
      text = (char*) malloc((++ntext + 1) * sizeof(char));
      if (!text)
        derp("unable to allocate token buffer");

      text[0] = fgetc(file.descriptor);
      text[1] = '\0';

      c = fgetc(file.descriptor);
      while ((c != '\"')) {
        text = (char*) realloc(text, (++ntext + 1) * sizeof(char));
        if (!text)
          derp("unable to reallocate token buffer");

        text[ntext - 1] = c;
        text[ntext] = '\0';

        c = fgetc(file.descriptor);
      }

      yylval->id = text;

      if (c == '\"')
        return ALIAS;

      free(text);
    }

    if (c == '%' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
      text = (char*) malloc((++ntext + 1) * sizeof(char));
      if (!text)
        derp("unable to allocate token buffer");

      text[0] = c;
      text[1] = '\0';

      c = fgetc(file.descriptor);
      while ((c >= 'a' && c <= 'z') ||
             (c >= 'A' && c <= 'Z') ||
             (c >= '0' && c <= '9') ||
              c == '_') {
        text = (char*) realloc(text, (++ntext + 1) * sizeof(char));
        if (!text)
          derp("unable to reallocate token buffer");

        text[ntext - 1] = c;
        text[ntext] = '\0';

        c = fgetc(file.descriptor);
      }

      fseek(file.descriptor, -1, SEEK_CUR);

      yylval->id = text;
      if (text[0] == '%') {
        if (strcmp(text, STR_EPSILON) == 0)
          return EPSILON;
        else if (strcmp(text, STR_TOKEN) == 0)
          return TOKEN;
        else
          free(text);
      }
      else
        return ID;
    }
  }
}

/* derp(): write an error message to stderr and end execution.
 */
void derp (const char *fmt, ...) {
  va_list vl;

  fprintf(stderr, "%s: error: ", argv0);

  va_start(vl, fmt);
  vfprintf(stderr, fmt, vl);
  va_end(vl);

  fprintf(stderr, "\n");
  fflush(stderr);
  exit(1);
}

/* main(): application entry point.
 */
int main (int argc, char **argv) {
	grammar_t g;

  symbols_init(&g);
  prods_init(&g);
  aliases_init(&g);

  argv0 = argv[0];

  if (argc != 2)
    derp("input filename required");

  file_t file = {
    .name = argv[1],
    .descriptor = fopen(argv[1], "r")
  };

  if (!file.descriptor)
    derp("%s: %s", file.name, strerror(errno));

  if (yyparse(file, &g))
    derp("%s: parse failed", file.name);

  fclose(file.descriptor);

  derives_empty(&g);
  first(&g);
  follow(&g);
  predict(&g);

  printf("Terminal symbols:\n\n");
  symbols_print(&g, 1);
  printf("\n");

  printf("Non-terminal symbols:\n\n");
  symbols_print(&g, 0);
  printf("\n");

  printf("Grammar:\n");
  prods_print(&g);
  printf("\n");

  printf("Empty derivations:\n\n");
  symbols_print_empty(&g);
  printf("\n");

  printf("First sets:\n\n");
  symbols_print_first(&g);

  printf("Follow sets:\n\n");
  symbols_print_follow(&g);

  printf("Predict sets:\n\n");
  prods_print_predict(&g);

  bool has_conflicts = conflicts(&g);

  aliases_free(&g);
  symbols_free(&g);
  prods_free(&g);

  return (has_conflicts) ? 1 : 0;
}
