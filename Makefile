# Author: Jack Robbins
# Makefile for ease of compilation, if preferred

CC = gcc
PROGS = regex_test
CFLAGS = -Wall -Wextra
INC = ./src/regex/regex.c ./src/stack/stack.c

regex_test: 
	$(CC) $(CFLAGS) ./src/main.c $(INC) -o ./out/regex_test

test:
	chmod +x ./out/regex_test
	./out/regex_test

clean:
	rm -r out/*


