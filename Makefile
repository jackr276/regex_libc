# Author: Jack Robbins
# Makefile for ease of compilation, if preferred

CC = gcc
PROGS = regex_test
CFLAGS = -Wall -Wextra
INC = ./src/regex/regex.c ./src/stack/stack.c
DEBUG_FLAG = -g

regex_test: 
	$(CC) $(CFLAGS) ./src/main.c $(INC) -o ./out/regex_test

regex_debug:
	$(CC) $(CFLAGS) $(DEBUG_FLAG) ./src/main.c $(INC) -o ./out/regex_debug

test:
	chmod +x ./out/regex_test
	./out/regex_test

clean:
	rm -r out/*


