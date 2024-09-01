/**
 * Author: Jack Robbins
 * This is an example program that will demonstrate the functionality of the regex library
 */

#include <stdio.h>
#include "regex.h"
#include "stack/stack.h"

int main(){

	stack_t* stack = create_stack();

	for(int i = 0; i < 100; i++){
		int j = i;
		int* b = &j;

		printf("Pushing: %d\n", *b);
		push(stack, b);
	}

	for(int i = 1; i < 32; i++){
		int* a = pop(stack);
		printf("%d\n", *a);
	}




}
