
CC=gcc
CFLAGS=-O0 -g -Wall -Wextra -std=c11

YACC=bison
YFLAGS=-d -Wall -Wdangling-alias

RM=rm -rf

BIN=ll1

all: $(BIN)

$(BIN): ll1.o grammar.o main.o
	@echo " LD   $@"
	@$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	@echo " CC   $^"
	@$(CC) $(CFLAGS) -o $@ -c $^

%.c: %.y
	@echo " YACC $^"
	@$(YACC) $(YFLAGS) -o $@ $^

clean:
	@echo " CLEAN"
	@$(RM) ll1.o ll1.c grammar.o ll1.h main.o
	@$(RM) $(BIN)
	@$(RM) $(BIN).dSYM

again: clean all

lines:
	@echo " WC"
	@wc -l $(YIN)
