/**
 * Author: Jack Robbins
 * This is an example program that will demonstrate the functionality of the regex library
 */

#include <stdio.h>
#include "regex/regex.h"
#include "stack/stack.h"

int main(){
	define_regular_expression("a(bb)+a", REGEX_VERBOSE);
}
