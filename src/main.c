/**
 * Author: Jack Robbins
 * This is an example program that will demonstrate the functionality of the regex library
 */

#include "regex/regex.h"
#include <stdio.h>

int main(){

	//TODO tester causes a bug when it should not
	regex_t tester_2 = define_regular_expression("a|b", REGEX_VERBOSE);

	char* string = "bbabcdbbbb";
	char* string2 = "I won't mAtch";

	//Attempt to match the string starting at 0
	regex_match_t rgx = regex_match(tester_2, string, 0, REGEX_VERBOSE);

	if(rgx.status == MATCH_FOUND){
		printf("Match found\n");
	}

//	destroy_regex(tester_2);
}
