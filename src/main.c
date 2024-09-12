/**
 * Author: Jack Robbins
 * This is an example program that will demonstrate the functionality of the regex library
 */

#include <stdio.h>
#include "regex/regex.h"
#include "stack/stack.h"

int main(){

	regex_t tester = define_regular_expression("abba|baab", REGEX_VERBOSE);

	destroy_regex(tester);
	

}
