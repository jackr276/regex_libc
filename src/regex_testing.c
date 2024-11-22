/**
 * Author: Jack Robbins
 * This is an example program that will demonstrate the functionality of the regex library
 */

#include "regex/regex.h"
#include <linux/limits.h>
#include <stdio.h>
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

	//Go through all of our test cases. Designed so that we can always add more
	switch(test_case){
		//Use fall through to run all of these
		case ALL:
			printf("Running ALL test cases:\n\n");
		//Case 1 tests regular concatenation 
		case 1:
			printf("Testing plain concatenation with regex: \"abcd\":\n");

			// Define tester
			tester = define_regular_expression("abcd", REGEX_VERBOSE);

			//Define a test string
			test_string = "aaa  b-b#bbbbabcdlmnop";
			printf("TEST STRING: %s\n", test_string);
			
			//Test the matching
			regex_match(tester, test_string, 0, REGEX_VERBOSE);

			//Destroy the regex
			destroy_regex(tester);

			//Break out if we don't fall through
			if(fall_through == 0){
				break;
			}
			
		//Case 2 tests the 0 or 1 ? operator
		case 2:
			printf("Testing concatenation with ? operator:\n");

			//Define the regex
			tester = define_regular_expression("abc?d", REGEX_VERBOSE);

			//Define a test string
			test_string = "aaabbbbbbabcdlmnop";
			printf("TEST STRING: %s\n", test_string);
			
			//Test the matching
			regex_match(tester, test_string, 0, REGEX_VERBOSE);

			//Define a test string
			test_string = "aaabbbbbbabdlmnop";
			printf("TEST STRING: %s\n", test_string);
	
			//Test the matching
			regex_match(tester, test_string, 0, REGEX_VERBOSE);
	
			//Destroy the regex
			destroy_regex(tester);

			//Break out if we don't fall through
			if(fall_through == 0){
				break;
			}
			
		//Case 3 tests using the escape character ~
		case 3:
			printf("Testing the explicit escape character ~:\n");

			test_string = "aaaaaaab(cd)a";
			//Define the regex
			tester = define_regular_expression("ab~(cd~)a", REGEX_VERBOSE);

			//Test the matching
			regex_match(tester, test_string, 0, REGEX_VERBOSE);
			
			//Ensure that this works
			destroy_regex(tester);

			//Break out if we don't fall through
			if(fall_through == 0){
				break;
			}
			
		case 4:
			printf("Testing concatenation with | operator:\n");

			//Define the regex
			tester = define_regular_expression("ab|d", REGEX_VERBOSE);

			//Define a test string
			test_string = "aaabbbbbbabcdlmnop";
			printf("TEST STRING: %s\n", test_string);
			
			//Test the matching
			regex_match(tester, test_string, 0, REGEX_VERBOSE);

			//Destroy the regex
			destroy_regex(tester);


			//Break out if we don't fall through
			if(fall_through == 0){
				break;
			}
			
		//Case 5 tests kleene star
		case 5:
			printf("Testing the * operator:\n");

			//Initialization
			tester = define_regular_expression("ab*c", REGEX_VERBOSE);

			//Define the test string
			test_string = "aaabbbbbbc";
			printf("TEST STRING: %s\n", test_string);
			
			//Test matching
			regex_match(tester, test_string, 0, REGEX_VERBOSE);
			
			//Destroy
			destroy_regex(tester);
	
			//Break out if we don't fall through
			if(fall_through == 0){
				break;
			}
		
		//Case 6 tests positive closure
		case 6:
			printf("Testing the + operator:\n");
			
			//Initialization
			tester = define_regular_expression("ab+c",REGEX_VERBOSE);
		
			//Define a test string
			test_string = "aaabbbbcd";

			//Test the matching, we should get one here
			regex_match(tester, test_string, 0,  REGEX_VERBOSE);
			
			//Destroy 
			destroy_regex(tester);
		
			//Break out if we don't fall through
			if(fall_through == 0){
				break;
			}
		
			
		//Case 6 tests positive closure
		case 7:
			printf("Testing the * operator alone:\n");
			
			//Initialization
			tester = define_regular_expression("aa*b",REGEX_VERBOSE);
		
			//Define a test string
			test_string = "aaabbbbcd";

			//Test the matching, we should get one here
			regex_match(tester, test_string, 0,  REGEX_VERBOSE);
			
			//Destroy 
			destroy_regex(tester);
		
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
	u_int32_t argument;

	if(argc == 2 && (sscanf(argv[1], "%u", &argument) > 0)){
		test_case_run(argument, 0);
	}
	
	if(argc == 1){
		test_case_run(ALL, 1);
	}

}
