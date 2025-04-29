
CC=gcc
CFLAGS=-g -Wall -Wextra -std=c99

YACC=bison
YFLAGS=-d

RM=rm -rf

BIN=ll1

all: $(BIN)

$(BIN): ll1.o grammar.o
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
	@$(RM) ll1.o ll1.c grammar.o
	@$(RM) $(BIN).dSYM

again: clean all

lines:
	@echo " WC"
	@wc -l $(YIN)
