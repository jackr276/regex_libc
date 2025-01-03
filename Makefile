# Author: Jack Robbins
# Makefile for ease of compilation, if preferred

CC = gcc
PROGS = regex_test
CFLAGS = -Wall -Wextra
INC = ./src/regex/regex.c ./src/stack/stack.c
DEBUG_FLAG = -g
OUT_DIR = ./out

regex_test: 
	$(CC) $(CFLAGS) ./src/regex_testing.c $(INC) -o $(OUT_DIR)/regex_test

regex_debug:
	$(CC) $(CFLAGS) $(DEBUG_FLAG) ./src/regex_testing.c $(INC) -o $(OUT_DIR)/regex_debug

test:
	chmod +x $(OUT_DIR)/regex_test
	#All currently passing make sure that we still pass
	$(OUT_DIR)/regex_test 1 
	$(OUT_DIR)/regex_test 3

test_all:
	chmod +x $(OUT_DIR)/regex_test
	# No args = all
	$(OUT_DIR)/regex_test 

clean:
	rm -f $(OUT_DIR)/*


