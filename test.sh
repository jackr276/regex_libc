# Author: Jack Robbins
# Runner/tester script


#!/bin/bash


if [[ ! -d ./out ]]; then
	mkdir out
fi

rm -r out/*

gcc -Wall -Wextra ./src/main.c ./src/regex/regex.c ./src/stack/stack.c -o ./out/run
