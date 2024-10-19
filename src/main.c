/**
 * Author: Jack Robbins
 * This is an example program that will demonstrate the functionality of the regex library
 */

#include "regex/regex.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#define ALL 0 


/**
* Define a testing function for us here. "test_case" is what case we want to test, and fall through
* let's us know that we want to test everything after that case
*/
void test_case_run(u_int8_t test_case, u_int8_t fall_through){
	regex_t tester;
	char* test_string;

	//If we're testing all cases then we will of course fall through
	if(test_case == ALL){
		fall_through = 1;
	}

	switch(test_case){
		case ALL:
			printf("Running ALL test cases:\n\n");
		//Case 1 tests 
		case 1:
			printf("Testing plain concatenation with regex: \"abcd\":\n");
			// Define tester
			tester = define_regular_expression("abcd", REGEX_VERBOSE);
			//Define a test string
			test_string = "aaabbbbbbabcdlmnop";
			printf("TEST STRING: %s\n", test_string);
			
			//Break out if we don't fall through
			if(fall_through == 0){
				break;
			}
			
		case 2:

			//Break out if we don't fall through
			if(fall_through == 0){
				break;
			}
			
		case 3:

			//Break out if we don't fall through
			if(fall_through == 0){
				break;
			}
			
		case 4:

			//Break out if we don't fall through
			if(fall_through == 0){
				break;
			}
			
		case 5:
	
			//Break out if we don't fall through
			if(fall_through == 0){
				break;
			}
				
		//Added to avoid comptime errors, we shouldn't reach this
		default:
			return;
	}
}

int main(int argc, char** argv){
	u_int8_t argument;

	if(argc == 2 && (argument = atoi(argv[1]) != 0)){
		test_case_run(argument, 0);
	}
	
	if(argc == 1){
		test_case_run(ALL, 1);
	}

}
