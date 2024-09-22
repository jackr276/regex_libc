/**
 * Author: Jack Robbins
 * This is an example program that will demonstrate the functionality of the regex library
 */

#include <stdio.h>
#include "regex/regex.h"
#include "stack/stack.h"

int main(){

	//TODO tester causes a bug when it should not
	regex_t tester_2 = define_regular_expression("a|b", REGEX_VERBOSE);

	char* string = "Match me";

	//Attempt to match the string starting at 0
	regex_match_t rgx = regex_match(tester_2, string, 0, REGEX_VERBOSE);

	

	destroy_regex(tester_2);
	

}
