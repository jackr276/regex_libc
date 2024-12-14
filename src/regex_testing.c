/**
 * Author: Jack Robbins
 * This is an example program that will demonstrate the functionality of the regex library
 */

#include "regex/regex.h"
#include "stack/stack.h"
#include <stdio.h>
#include <sys/types.h>


/**
* Define a testing function for us here. "test_case" is what case we want to test, and fall through
* let's us know that we want to test everything after that case
*/
void test_case_run(u_int8_t test_case){
	char* test_string;
	regex_t* tester;

	//Go through all of our test cases. Designed so that we can always add more
	switch(test_case){
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

			//Define a test string -- should fail
			test_string = "aaa  b-b#bbbbabclmnop";
			printf("TEST STRING: %s\n", test_string);
			
			//Test the matching
			regex_match(tester, test_string, 0, REGEX_VERBOSE);

			//Destroy the regex
			destroy_regex(tester);

			//Break out if we don't fall through
			return;
			
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
			return;
			
		//Case 3 tests using the escape character ~
		case 3:
			printf("Testing the explicit escape character \\:\n");

			test_string = "aaaaaaab(cd)a";
			printf("TEST STRING: %s\n", test_string);
			//Define the regex
			tester = define_regular_expression("ab\\(cd\\)a", REGEX_VERBOSE);

			//Test the matching
			regex_match(tester, test_string, 0, REGEX_VERBOSE);
			
			//Ensure that this works
			destroy_regex(tester);

			return;
			
		case 4:
			printf("Testing concatenation with | operator:\n");

			//Define the regex
			tester = define_regular_expression("ab|d", REGEX_VERBOSE);

			//Define a test string
			test_string = "aaabbbbbbabcdlmnop";
			printf("TEST STRING: %s\n", test_string);
			
			//Test the matching
			regex_match(tester, test_string, 0, REGEX_VERBOSE);

			//Define a test string
			test_string = "aacbbbbbbacdlmnop";
			printf("TEST STRING: %s\n", test_string);
			
			//Test the matching
			regex_match(tester, test_string, 0, REGEX_VERBOSE);

			//Destroy the regex
			destroy_regex(tester);
	
			return;
			
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
	
			return;
		
		//Case 6 tests positive closure
		case 6:
			printf("Testing the + operator:\n");
			
			//Initialization
			tester = define_regular_expression("ab+c", REGEX_VERBOSE);
		
			//Define a test string
			test_string = "aaabbbbcd";

			//Test the matching, we should get one here
			regex_match(tester, test_string, 0,  REGEX_VERBOSE);

			//Define a test string
			test_string = "aaacd";

			//Test the matching, we should fail here
			regex_match(tester, test_string, 0,  REGEX_VERBOSE);

			//Destroy 
			destroy_regex(tester);
		
			//Break out if we don't fall through
			return;
		
			
		//Case 7 tests positive closure
		case 7:
			printf("Testing the * operator alone:\n");
			
			//Initialization
			tester = define_regular_expression("aa*b", REGEX_VERBOSE);
		
			//Define a test string
			test_string = "aaabbbbcd";

			//Test the matching, we should get one here
			regex_match(tester, test_string, 0,  REGEX_VERBOSE);
			
			//Destroy 
			destroy_regex(tester);
		
			return;

		case 8:
			printf("Testing associativity\n");
			
			//Initialization
			tester = define_regular_expression("a(bc)*", REGEX_VERBOSE);

			//We should match here
			test_string = "bcdabcbcbcbcbcd";

			regex_match(tester, test_string, 0, REGEX_VERBOSE);
			
			destroy_regex(tester);
				
			return;

		case 9:
			printf("More associativity tests\n");

			//Initialization
			tester = define_regular_expression("a(bc)?d", REGEX_VERBOSE);

			//We should match
			test_string = "zyxwvutabcdlmnop";
			regex_match(tester, test_string, 0, REGEX_VERBOSE);
			
			destroy_regex(tester);
			
			return;
				
		case 10:
			printf("Chaining Kleene Splits\n");

			
			//Initialization
			tester = define_regular_expression("a(bc)*dlmnop*d", REGEX_VERBOSE);

			//Should have a match here
			test_string = "asdklf;asdfabcbcdlmnopd";
			
			regex_match(tester, test_string, 0, REGEX_VERBOSE);
		
			destroy_regex(tester);	
			
			return;
		
			
		case 11:
			printf("More alternation tests\n");

		/**
		 * TODO this still segfaults, some issue with NFA creation
		*/
			//Initialization
			printf("Regex: (ab|da)bc\n");
			//Does nothing for now due to brokenness
			tester = define_regular_expression("(ab|da)bc", REGEX_VERBOSE);
			return;
		
		return;

		case 12:
			printf("Chaining Positive Closures\n");

			
			//Initialization
			tester = define_regular_expression("a(bc)+dlmnop+d", REGEX_VERBOSE);

			//Should have a match here
			test_string = "asdklf;asdfabcdlmnopd";
			
			regex_match(tester, test_string, 0, REGEX_VERBOSE);
		
			destroy_regex(tester);	
			
			return;

		case 13:
			printf("Chaining Zero or one operators\n");
			
			//Initialization
			tester = define_regular_expression("ab?cdef(ge)?a", REGEX_VERBOSE);
			
			test_string = "asdfasfdacdefgeakjs";

			//We should have a match
			regex_match(tester, test_string, 0, REGEX_VERBOSE);

			//Clean up
			destroy_regex(tester);
			
			return;

		case 14:
			printf("Combining Zero or one and kleene");
			
			//Initialization
			tester = define_regular_expression("ab?cdef(ge)*a", REGEX_VERBOSE);
			
			test_string = "asdfasfdacdefgegeakjs";

			//We should have a match
			regex_match(tester, test_string, 0, REGEX_VERBOSE);

			//Clean up
			destroy_regex(tester);

			return;
			
		case 15:
			printf("Combining Zero or one and kleene");
			
			//Initialization
			tester = define_regular_expression("ab*cdef(ge)?a", REGEX_VERBOSE);
			
			test_string = "as   --*dfasfdacdefgeakjs";

			//We should have a match
			regex_match(tester, test_string, 0, REGEX_VERBOSE);

			//Clean up
			destroy_regex(tester);
			
			return;

		case 16:
			printf("Combining alternation and kleene");

			//Initialization
			tester = define_regular_expression("abc|de*f", REGEX_VERBOSE);

			test_string = "aaabbbbbbbbbbbbbbcasdfasd";
	
			//We should have a match
			regex_match(tester, test_string, 0, REGEX_VERBOSE);

			test_string = "acbcdbdefasfa";

			//We should have a match
			regex_match(tester, test_string, 0, REGEX_VERBOSE);

			destroy_regex(tester);

			return;
				
		case 17:
			printf("Testing nesting parenthesis\n");
			printf("REGEX: a(bcd(ab)*)?efg\n");

			//Initialization
			tester = define_regular_expression("a(bcd(ab)*)?efg", REGEX_VERBOSE);
			destroy_regex(tester);

			return;
			
		
		//Added to avoid comptime errors, we shouldn't reach this
		default:
			return;
	}
}

int main(int argc, char** argv){
	u_int32_t argument;

	if(argc == 2 && (sscanf(argv[1], "%u", &argument) > 0)){
		test_case_run(argument);
	}
	
	if(argc == 1){
		for(u_int8_t i = 0; i < 16; i++){
			test_case_run(i);
		}
	}

}
