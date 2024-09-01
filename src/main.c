/**
 * Author: Jack Robbins
 * This is an example program that will demonstrate the functionality of the regex library
 */

#include <stdio.h>
#include "regex/regex.h"
#include "stack/stack.h"

int main(){

	stack_t* stack = create_stack();

	char* a = "Hello";
	char* b = "World";

	push(stack, b);
	push(stack, a);

	printf("%s ", (char*)pop(stack));
	printf("%s\n", (char*)pop(stack));

	define_regular_expression("ex");


	destroy_stack(stack);


}
