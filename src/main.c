/**
 * Author: Jack Robbins
 * This is an example program that will demonstrate the functionality of the regex library
 */

#include <stdio.h>
#include "regex/regex.h"
#include "stack/stack.h"

int main(){

	//TODO tester causes a bug when it should not
	regex_t tester = define_regular_expression("(abba|baab)|(b*a?i+)", REGEX_VERBOSE);
	regex_t tester_2 = define_regular_expression("a|b", REGEX_VERBOSE);

	destroy_regex(tester);
	destroy_regex(tester_2);
	

}
