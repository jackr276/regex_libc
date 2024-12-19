/**
 * Author: Jack Robbins
 * This is an example program that will demonstrate the functionality of the regex library
 */

#include "regex/regex.h"
#include <stdio.h>
#include <sys/types.h>


/**
* Define a testing function for us here. "test_case" is what case we want to test, and fall through
* let's us know that we want to test everything after that case
*/
void test_case_run(u_int8_t test_case){
	char* test_string;
	regex_t* tester;
	regex_match_t matcher;

	//Go through all of our test cases. Designed so that we can always add more
	switch(test_case){
		//Case 1 tests regular concatenation 
		case 1:
			printf("Testing plain concatenation with regex\n");
			printf("REGEX: 'abcd'\n");

			// Define tester
			tester = define_regular_expression("abcd", REGEX_VERBOSE);

			//Define a test string
			test_string = "aaa  b-b#bbbbabcdlmnop";
			printf("TEST STRING: %s\n\n", test_string);
			
			//Test the matching
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			//Define a test string -- should fail
			test_string = "aaa  b-b#bbbbabclmnop";
			printf("TEST STRING: %s\n\n", test_string);
			
			//Test the matching
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			//Destroy the regex
			destroy_regex(tester);

			//Break out if we don't fall through
			return;
			
		//Case 2 tests the 0 or 1 ? operator
		case 2:
			printf("Testing concatenation with ? operator:\n");
			printf("REGEX: 'abc?d'\n\n");

			//Define the regex
			tester = define_regular_expression("abc?d", REGEX_VERBOSE);

			//Define a test string
			test_string = "aaabbbbbbabcdlmnop";
			printf("TEST STRING: %s\n", test_string);
			
			//Test the matching
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			//Define a test string
			test_string = "aaabbbbbbabdlmnop";
			printf("TEST STRING: %s\n\n", test_string);
	
			//Test the matching
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);
	
			//Destroy the regex
			destroy_regex(tester);

			//Break out if we don't fall through
			return;
			
		//Case 3 tests using the escape character ~
		case 3:
			printf("Testing the explicit escape character \\:\n");
			printf("REGEX: 'ab\\(cd\\k)a'\n");

			test_string = "aaaaaaab(cd)a";
			printf("TEST STRING: %s\n\n", test_string);
			//Define the regex
			tester = define_regular_expression("ab\\(cd\\)a", REGEX_VERBOSE);

			//Test the matching
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);
			
			//Ensure that this works
			destroy_regex(tester);

			return;
			
		case 4:
			printf("Testing concatenation with | operator:\n");
			printf("REGEX: 'ab|d'\n");

			//Define the regex
			tester = define_regular_expression("ab|d", REGEX_VERBOSE);

			//Define a test string
			test_string = "aaabbbbbbabcdlmnop";
			printf("TEST STRING: %s\n\n", test_string);
			
			//Test the matching
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			//Define a test string
			test_string = "aacbbbbbbacdlmnop";
			printf("TEST STRING: %s\n\n", test_string);
			
			//Test the matching
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			//Destroy the regex
			destroy_regex(tester);
	
			return;
			
		//Case 5 tests kleene star
		case 5:
			printf("Testing the * operator:\n");
			printf("REGEX: 'ab*c'\n");

			//Initialization
			tester = define_regular_expression("ab*c", REGEX_VERBOSE);

			//Define the test string
			test_string = "aaabbbbbbc a.kas";
			printf("TEST STRING: %s\n\n", test_string);
			
			//Test matching
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);
			
			//Destroy
			destroy_regex(tester);
	
			return;
		
		//Case 6 tests positive closure
		case 6:
			printf("Testing the + operator:\n");
			printf("REGEX: 'ab+c'\n");
			
			//Initialization
			tester = define_regular_expression("ab+c", REGEX_VERBOSE);
		
			//Define a test string
			test_string = "aaabbbbcd";
			printf("TEST STRING: %s\n\n", test_string);

			//Test the matching, we should get one here
			regex_match(tester, &matcher, test_string, 0,  REGEX_VERBOSE);

			//Define a test string
			test_string = "aaacd";
			printf("TEST STRING: %s\n\n", test_string);

			//Test the matching, we should fail here
			regex_match(tester, &matcher, test_string, 0,  REGEX_VERBOSE);

			//Destroy 
			destroy_regex(tester);
		
			//Break out if we don't fall through
			return;
		
			
		//Case 7 tests positive closure
		case 7:
			printf("Testing the * operator alone:\n");
			printf("REGEX: 'aa*b'\n");

			//Initialization
			tester = define_regular_expression("aa*b", REGEX_VERBOSE);
		
			//Define a test string
			test_string = "aaabbbbcd";
			printf("TEST STRING: %s\n\n", test_string);

			//Test the matching, we should get one here
			regex_match(tester, &matcher, test_string, 0,  REGEX_VERBOSE);
			
			//Destroy 
			destroy_regex(tester);
		
			return;

		case 8:
			printf("Testing associativity\n");
			printf("REGEX: 'a(bc)*'\n");

			//Initialization
			tester = define_regular_expression("a(bc)*", REGEX_VERBOSE);

			//We should match here
			test_string = "bcdabcbcbcbcbcd";
			printf("TEST STRING: %s\n\n", test_string);

			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);
			
			destroy_regex(tester);
				
			return;

		case 9:
			printf("More associativity tests\n");
			printf("REGEX: 'a(bc)?d'\n");

			//Initialization
			tester = define_regular_expression("a(bc)?d", REGEX_VERBOSE);

			//We should match
			test_string = "zyxwvutabcdlmnop";
			printf("TEST STRING: %s\n\n", test_string);
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);
	

			//We should match
			test_string = "zyxwvutabcdlmnop";
			printf("TEST STRING: %s\n\n", test_string);
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);
			
		
			destroy_regex(tester);
			
			return;
				
		case 10:
			printf("Chaining Kleene Splits\n");

			
			//Initialization
			tester = define_regular_expression("a(bc)*dlmnop*d", REGEX_VERBOSE);

			//Should have a match here
			test_string = "asdklf;asdfabcbcdlmnopd";
			printf("TEST STRING: %s\n\n", test_string);
			
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);
		
			destroy_regex(tester);	
			
			return;
			
		case 11:
			printf("More alternation tests\n");

			//Initialization
			printf("Regex: (ab|da)bc\n");
			//Does nothing for now due to brokenness
			tester = define_regular_expression("(ab|da)bc", REGEX_VERBOSE);

			//This should match
			test_string = "aaaaaaabbcd";
			printf("TEST STRING: %s\n\n", test_string);

			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			//This should match
			test_string = "aaaaaadabcd";
			printf("TEST STRING: %s\n\n", test_string);

			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			//This should not match 
			test_string = "asfdasdfabdabcda";
			printf("TEST STRING: %s\n\n", test_string);

			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			destroy_regex(tester);

			return;
		
		case 12:
			printf("Chaining Positive Closures\n");
			
			//Initialization
			tester = define_regular_expression("a(bc)+dlmnop+d", REGEX_VERBOSE);

			//Should have a match here
			test_string = "asdklf;asdfabcbcdlmnoppppppdassd";
			printf("TEST STRING: %s\n\n", test_string);
			
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);
		
			destroy_regex(tester);	
			
			return;

		case 13:
			printf("Chaining Zero or one operators\n");
			
			//Initialization
			tester = define_regular_expression("ab?cdef(ge)?a", REGEX_VERBOSE);
			
			test_string = "asdfasfdacdefgeakjs";
			printf("TEST STRING: %s\n\n", test_string);

			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			//Clean up
			destroy_regex(tester);
			
			return;

		case 14:
			printf("Combining Zero or one and kleene");
			
			//Initialization
			tester = define_regular_expression("ab?cdef(ge)*a", REGEX_VERBOSE);
			
			test_string = "asdfasfdacdefgegeakjs";
			printf("TEST STRING: %s\n\n", test_string);

			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			//Clean up
			destroy_regex(tester);

			return;
			
		case 15:
			printf("Combining Zero or one and kleene");
			
			//Initialization
			tester = define_regular_expression("ab*cdef(ge)?a", REGEX_VERBOSE);
			
			test_string = "as   --*dfasfdacdefgeakjs";
			printf("TEST STRING: %s\n\n", test_string);

			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			//Clean up
			destroy_regex(tester);
			
			return;

		case 16:
			printf("Combining alternation and kleene\n");

			//Initialization
			tester = define_regular_expression("abc|de*f", REGEX_VERBOSE);

			test_string = "aaabbbbbbbbbbbbbbcasdfasd";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			test_string = "acbcdbdefasfa";
			printf("TEST STRING: %s\n\n", test_string);

			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			destroy_regex(tester);

			return;
				
		case 17:
			printf("Testing nesting parenthesis\n");
			printf("REGEX: a(bcd(ab)*)?efg\n");

			//Initialization
			tester = define_regular_expression("a(bcd(ab)*)?efg", REGEX_VERBOSE);

			test_string = "sdaefgdefabcd";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			test_string = "aaabcdababababefgdfa";
			printf("TEST STRING: %s\n\n", test_string);

			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			destroy_regex(tester);


			return;

		case 18:
			printf("Testing nesting parenthesis\n");
			printf("REGEX: (a|b)(c|d)a\n");

			//Initialization
			tester = define_regular_expression("((a|b)(c|d))a", REGEX_VERBOSE);

			test_string = "aaabbbbbbbbbbbbbbcasdfasd";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			test_string = "cbcabdefasfa";
			printf("TEST STRING: %s\n\n", test_string);

			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			destroy_regex(tester);

			return;

		case 19:
			printf("Testing nesting parenthesis\n");
			printf("REGEX: ((ab*a)|(gef))d\n");

			//Initialization
			tester = define_regular_expression("((ab*a)|(gef))d", REGEX_VERBOSE);

			test_string = "adddabbbbbbbbbbbbbbadasdfasd";
			printf("TEST STRING: %s\n\n", test_string);

			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			test_string = "sdafasdfgefdas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);


			destroy_regex(tester);

			return;

		case 20:
			printf("Testing concatenation with parenthesis\n");
			printf("REGEX: (a|c)b\n");

			//Initialization
			tester = define_regular_expression("(a|c)b", REGEX_VERBOSE);

			test_string = "aaabbbbbbbbbbbbbbcasdfasd";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			test_string = "aacbbbbbbbbbbbbbbcasdfasd";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);


			destroy_regex(tester);

			return;
			
		case 21:
			printf("Testing parenthesization");
			printf("REGEX: (ab(cd)bcd)(aflf)\n");

			//Initialization
			tester = define_regular_expression("(ab(cd)bcd)(aflf)", REGEX_VERBOSE);

			test_string = "aaaaavabcdbcdaflfas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			destroy_regex(tester);

			return;

		case 22:
			printf("Testing parenthesization with kleene");
			printf("REGEX: (ab(cd)*bcd)(aflf)\n");

			//Initialization
			tester = define_regular_expression("(ab(cd)*bcd)(aflf)", REGEX_VERBOSE);

			test_string = "aaaaavabcdcdcdcdcdcdcdbcdaflfas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			destroy_regex(tester);

			return;

		case 23:
			printf("Testing parenthesization with kleene");
			printf("REGEX: l(ab(cd)bcd)*(flf)\n");

			//Initialization
			tester = define_regular_expression("l(ab(cd)bcd)*(flf)", REGEX_VERBOSE);

			test_string = "aaaaavlabcdbcdabcdbcdflfas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			destroy_regex(tester);

			return;
	
		//Not working
		case 24:
			printf("Testing parenthesization with kleene and positive closure\n");
			printf("REGEX: (ab(cd)*bcd)+(flf)\n");

			//Initialization
			tester = define_regular_expression("(ab(cd)*bcd)+(flf)", REGEX_VERBOSE);

			test_string = "aaaaavabcdbcdabcdbcdflfas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			test_string = "aaaaavabbcdabcdbcdflfas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);


			destroy_regex(tester);

			return;

		case 25:
			printf("Testing parenthesization with kleene\n");
			printf("REGEX: (ab(cd)bcd)+(flf)*\n");

			//Initialization
			tester = define_regular_expression("(ab(cd)bcd)+(flf)+", REGEX_VERBOSE);

			test_string = "aaaaavabcdbcdabcdbcdflfas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			test_string = "aaaaavabbcdabcdbcdflfas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);


			destroy_regex(tester);

			return;

		case 26:
			printf("Testing nesting parenthesis\n");
			printf("REGEX: ((gef)|(ab*a))d\n");

			//Initialization
			tester = define_regular_expression("((gef)|(ab*a))d", REGEX_VERBOSE);

			test_string = "adddabbbbbbbbbbbbbbadasdfasd";
			printf("TEST STRING: %s\n\n", test_string);

			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			test_string = "sdafasdfgefdas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			destroy_regex(tester);

			return;

		case 27:
			printf("Testing parenthesization with kleene and positive closure\n");
			printf("REGEX: (ab(cd)*bcd)+e\n");

			//Initialization
			tester = define_regular_expression("(ab(ef)*bcd)+e", REGEX_VERBOSE);

			test_string = "aaaaavabefefefefefbcdabbcdeflfas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			test_string = "aaaaavabbcdabcdbcdflfas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			destroy_regex(tester);

			return;

		case 28:
			printf("Testing parenthesization with kleene\n");
			printf("REGEX: (ab(cd)*bcd)+\n");

			//Initialization
			tester = define_regular_expression("(ab(ef)*bcd)+", REGEX_VERBOSE);

			test_string = "aaaaavabefefefefefbcdbbbcdeflfas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			test_string = "aaaaavabbcdabbcdbcdflfas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);


			destroy_regex(tester);

			return;

		/**
		 * We will now turn off verbose mode and test everything to see if we have proper 
		 * match indices
		 */

		//Case 1 tests regular concatenation 
		case 29:
			printf("Testing plain concatenation with regex\n");
			printf("REGEX: 'abcd'\n");

			// Define tester
			tester = define_regular_expression("abcd", REGEX_SILENT);

			//Define a test string
			test_string = "aaa  b-b#bbbbabcdlmnop";
			printf("TEST STRING: %s\n\n", test_string);
			
			//Test the matching
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			//Define a test string -- should fail
			test_string = "aaa  b-b#bbbbabclmnop";
			printf("TEST STRING: %s\n\n", test_string);
			
			//Test the matching
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			//Destroy the regex
			destroy_regex(tester);

			//Break out if we don't fall through
			return;
			
		//Case 2 tests the 0 or 1 ? operator
		case 30:
			printf("Testing concatenation with ? operator:\n");
			printf("REGEX: 'abc?d'\n\n");

			//Define the regex
			tester = define_regular_expression("abc?d", REGEX_SILENT);

			//Define a test string
			test_string = "aaabbbbbbabcdlmnop";
			printf("TEST STRING: %s\n", test_string);
			
			//Test the matching
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			//Define a test string
			test_string = "aaabbbbbbabdlmnop";
			printf("TEST STRING: %s\n\n", test_string);
	
			//Test the matching
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			//Destroy the regex
			destroy_regex(tester);

			return;
			
		//Case 3 tests using the escape character ~
		case 31:
			printf("Testing the explicit escape character \\:\n");
			printf("REGEX: 'ab\\(cd\\k)a'\n");

			test_string = "aaaaaaab(cd)a";
			printf("TEST STRING: %s\n\n", test_string);
			//Define the regex
			tester = define_regular_expression("ab\\(cd\\)a", REGEX_SILENT);

			//Test the matching
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			//Ensure that this works
			destroy_regex(tester);

			return;
			
		case 32:
			printf("Testing concatenation with | operator:\n");
			printf("REGEX: 'ab|d'\n");

			//Define the regex
			tester = define_regular_expression("ab|d", REGEX_SILENT);

			//Define a test string
			test_string = "aaabbbbbbabcdlmnop";
			printf("TEST STRING: %s\n\n", test_string);
			
			//Test the matching
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			//Define a test string
			test_string = "aacbbbbbbacdlmnop";
			printf("TEST STRING: %s\n\n", test_string);
			
			//Test the matching
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			//Destroy the regex
			destroy_regex(tester);
	
			return;
			
		//Case 5 tests kleene star
		case 33:
			printf("Testing the * operator:\n");
			printf("REGEX: 'ab*c'\n");

			//Initialization
			tester = define_regular_expression("ab*c", REGEX_SILENT);

			//Define the test string
			test_string = "aaabbbbbbc a.kas";
			printf("TEST STRING: %s\n\n", test_string);
			
			//Test matching
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			//Destroy
			destroy_regex(tester);
	
			return;
		
		//Case 6 tests positive closure
		case 34:
			printf("Testing the + operator:\n");
			printf("REGEX: 'ab+c'\n");
			
			//Initialization
			tester = define_regular_expression("ab+c", REGEX_SILENT);
		
			//Define a test string
			test_string = "aaabbbbcd";
			printf("TEST STRING: %s\n\n", test_string);

			//Test the matching, we should get one here
			regex_match(tester, &matcher, test_string, 0,  REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			//Define a test string
			test_string = "aaacd";
			printf("TEST STRING: %s\n\n", test_string);

			//Test the matching, we should fail here
			regex_match(tester, &matcher, test_string, 0,  REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			destroy_regex(tester);
		
			//Break out if we don't fall through
			return;
		
			
		//Case 7 tests positive closure
		case 35:
			printf("Testing the * operator alone:\n");
			printf("REGEX: 'aa*b'\n");

			//Initialization
			tester = define_regular_expression("aa*b", REGEX_SILENT);
		
			//Define a test string
			test_string = "aaabbbbcd";
			printf("TEST STRING: %s\n\n", test_string);

			//Test the matching, we should get one here
			regex_match(tester, &matcher, test_string, 0,  REGEX_SILENT);
	
			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			//Destroy 
			destroy_regex(tester);
		
			return;

		case 36:
			printf("Testing associativity\n");
			printf("REGEX: 'a(bc)*'\n");

			//Initialization
			tester = define_regular_expression("a(bc)*", REGEX_SILENT);

			//We should match here
			test_string = "bcdabcbcbcbcbcd";
			printf("TEST STRING: %s\n\n", test_string);

			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}
	
			destroy_regex(tester);
				
			return;

		case 37:
			printf("More associativity tests\n");
			printf("REGEX: 'a(bc)?d'\n");

			//Initialization
			tester = define_regular_expression("a(bc)?d", REGEX_SILENT);

			//We should match
			test_string = "zyxwvutabcdlmnop";
			printf("TEST STRING: %s\n\n", test_string);

			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}
	
			//We should match
			test_string = "zyxwvutabcdlmnop";
			printf("TEST STRING: %s\n\n", test_string);

			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}
	
			destroy_regex(tester);
			
			return;
				
		case 38:
			printf("Chaining Kleene Splits\n");

			
			//Initialization
			tester = define_regular_expression("a(bc)*dlmnop*d", REGEX_SILENT);

			//Should have a match here
			test_string = "asdklf;asdfabcbcdlmnopd";
			printf("TEST STRING: %s\n\n", test_string);
			
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			destroy_regex(tester);	
			
			return;
			
		case 39:
			printf("More alternation tests\n");

			//Initialization
			printf("Regex: (ab|da)bc\n");
			//Does nothing for now due to brokenness
			tester = define_regular_expression("(ab|da)bc", REGEX_SILENT);

			//This should match
			test_string = "aaaaaaabbcd";
			printf("TEST STRING: %s\n\n", test_string);

			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			//This should match
			test_string = "aaaaaadabcd";
			printf("TEST STRING: %s\n\n", test_string);

			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			//This should not match 
			test_string = "asfdasdfabdabcda";
			printf("TEST STRING: %s\n\n", test_string);

			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			destroy_regex(tester);

			return;
		
		case 40:
			printf("Chaining Positive Closures\n");

			
			//Initialization
			tester = define_regular_expression("a(bc)+dlmnop+d", REGEX_SILENT);

			//Should have a match here
			test_string = "asdklf;asdfabcbcdlmnoppppppdassd";
			printf("TEST STRING: %s\n\n", test_string);
			
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);
			printf("%d\n", matcher.status);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			destroy_regex(tester);	
			
			return;

		case 41:
			printf("Chaining Zero or one operators\n");
			
			//Initialization
			tester = define_regular_expression("ab?cdef(ge)?a", REGEX_SILENT);
			
			test_string = "asdfasfdacdefgeakjs";
			printf("TEST STRING: %s\n\n", test_string);

			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			//Clean up
			destroy_regex(tester);
			
			return;

		case 42:
			printf("Combining Zero or one and kleene");
			
			//Initialization
			tester = define_regular_expression("ab?cdef(ge)*a", REGEX_SILENT);
			
			test_string = "asdfasfdacdefgegeakjs";
			printf("TEST STRING: %s\n\n", test_string);

			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			//Clean up
			destroy_regex(tester);

			return;
			
		case 43:
			printf("Combining Zero or one and kleene");
			
			//Initialization
			tester = define_regular_expression("ab*cdef(ge)?a", REGEX_SILENT);
			
			test_string = "as   --*dfasfdacdefgeakjs";
			printf("TEST STRING: %s\n\n", test_string);

			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			//Clean up
			destroy_regex(tester);
			
			return;

		case 44:
			printf("Combining alternation and kleene\n");

			//Initialization
			tester = define_regular_expression("abc|de*f", REGEX_SILENT);

			test_string = "aaabbbbbbbbbbbbbbcasdfasd";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			test_string = "acbcdbdefasfa";
			printf("TEST STRING: %s\n\n", test_string);

			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			destroy_regex(tester);

			return;
				
		case 45:
			printf("Testing nesting parenthesis\n");
			printf("REGEX: a(bcd(ab)*)?efg\n");

			//Initialization
			tester = define_regular_expression("a(bcd(ab)*)?efg", REGEX_SILENT);

			test_string = "sdaefgdefabcd";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			test_string = "aaabcdababababefgdfa";
			printf("TEST STRING: %s\n\n", test_string);

			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			destroy_regex(tester);

			return;

		case 46:
			printf("Testing nesting parenthesis\n");
			printf("REGEX: (a|b)(c|d)a\n");

			//Initialization
			tester = define_regular_expression("((a|b)(c|d))a", REGEX_SILENT);

			test_string = "aaabbbbbbbbbbbbbbcasdfasd";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			test_string = "cbcabdefasfa";
			printf("TEST STRING: %s\n\n", test_string);

			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			destroy_regex(tester);

			return;

		case 47:
			printf("Testing nesting parenthesis\n");
			printf("REGEX: ((ab*a)|(gef))d\n");

			//Initialization
			tester = define_regular_expression("((ab*a)|(gef))d", REGEX_SILENT);

			test_string = "adddabbbbbbbbbbbbbbadasdfasd";
			printf("TEST STRING: %s\n\n", test_string);

			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			test_string = "sdafasdfgefdas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			destroy_regex(tester);

			return;

		case 48:
			printf("Testing concatenation with parenthesis\n");
			printf("REGEX: (a|c)b\n");

			//Initialization
			tester = define_regular_expression("(a|c)b", REGEX_SILENT);

			test_string = "aaabbbbbbbbbbbbbbcasdfasd";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			test_string = "aacbbbbbbbbbbbbbbcasdfasd";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);
			
			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			destroy_regex(tester);

			return;
			
		case 49:
			printf("Testing parenthesization");
			printf("REGEX: (ab(cd)bcd)(aflf)\n");

			//Initialization
			tester = define_regular_expression("(ab(cd)bcd)(aflf)", REGEX_SILENT);

			test_string = "aaaaavabcdbcdaflfas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);
	
			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			destroy_regex(tester);

			return;

		case 50:
			printf("Testing parenthesization with kleene");
			printf("REGEX: (ab(cd)*bcd)(aflf)\n");

			//Initialization
			tester = define_regular_expression("(ab(cd)*bcd)(aflf)", REGEX_SILENT);

			test_string = "aaaaavabcdcdcdcdcdcdcdbcdaflfas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);
	
			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			destroy_regex(tester);

			return;

		case 51:
			printf("Testing parenthesization with kleene");
			printf("REGEX: l(ab(cd)bcd)*(flf)\n");

			//Initialization
			tester = define_regular_expression("l(ab(cd)bcd)*(flf)", REGEX_SILENT);

			test_string = "aaaaavlabcdbcdabcdbcdflfas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);
	
			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			destroy_regex(tester);

			return;
	
		//Not working
		case 52:
			printf("Testing parenthesization with kleene");
			printf("REGEX: (ab(cd)*bcd)+(flf)\n");

			//Initialization
			tester = define_regular_expression("(ab(cd)*bcd)+(flf)", REGEX_SILENT);

			test_string = "aaaaavabcdbcdabcdbcdflfas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			test_string = "aaaaavabbcdabcdbcdflfas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			destroy_regex(tester);

			return;

		case 53:
			printf("Testing parenthesization with positive closure\n");
			printf("REGEX: (ab(cd)bcd)+(flf)*\n");

			//Initialization
			tester = define_regular_expression("(ab(cd)bcd)+(flf)+", REGEX_SILENT);

			test_string = "aaaaavabcdbcdabcdbcdflfas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			test_string = "aaaaavabbcdabcdbcdflfas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			destroy_regex(tester);

			return;

		case 54:
			printf("Testing nesting parenthesis\n");
			printf("REGEX: ((gef)|(ab*a))d\n");

			//Initialization
			tester = define_regular_expression("((gef)|(ab*a))d", REGEX_SILENT);

			test_string = "adddabbbbbbbbbbbbbbadasdfasd";
			printf("TEST STRING: %s\n\n", test_string);

			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			test_string = "sdafasdfgefdas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);
			
			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			destroy_regex(tester);

			return;

		case 55:
			printf("Testing parenthesization with kleene\n");
			printf("REGEX: (ab(cd)*bcd)+e\n");

			//Initialization
			tester = define_regular_expression("(ab(ef)*bcd)+e", REGEX_SILENT);

			test_string = "aaaaavabefefefefefbcdabbcdeflfas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);
	
			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			test_string = "aaaaavabbcdabcdbcdflfas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);
	
			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			destroy_regex(tester);

			return;

		case 56:
			printf("Testing parenthesization with kleene\n");
			printf("REGEX: (ab(cd)*bcd)+\n");

			//Initialization
			tester = define_regular_expression("(ab(ef)*bcd)+", REGEX_SILENT);

			test_string = "aaaaavabefefefefefbcdbbbcdeflfas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			test_string = "aaaaavabbcdabbcdbcdflfas";
			printf("TEST STRING: %s\n\n", test_string);
	
			//We should have a match
			regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

			//Display if we've found a match
			if(matcher.status == MATCH_FOUND){
				printf("Match starts at index: %d and ends at index:%d\n\n", matcher.match_start_idx, matcher.match_end_idx);
			} else {
				printf("No match.\n\n");
			}

			destroy_regex(tester);

			return;

		//Wildcard character
		case 57:
			printf("Testing wildcard character\n");
			printf("REGEX: 'a$cd'\n");

			// Define tester
			tester = define_regular_expression("a$cd", REGEX_VERBOSE);

			//Define a test string
			test_string = "aaa  b-b#bbbbascdlmnop";
			printf("TEST STRING: %s\n\n", test_string);
			
			//Test the matching
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			//Define a test string -- should fail
			test_string = "aaa  b-b#bbbbabclmnop";
			printf("TEST STRING: %s\n\n", test_string);
			
			//Test the matching
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			//Destroy the regex
			destroy_regex(tester);

			//Break out if we don't fall through
			return;
	
		//Wildcard character
		case 58:
			printf("Testing wildcard character\n");
			printf("REGEX: 'a(l$l)*cd'\n");

			// Define tester
			tester = define_regular_expression("a(l$l)*cd", REGEX_VERBOSE);

			//Define a test string
			test_string = "aaa  b-b#bbbbalollalldlcdlmnop";
			printf("TEST STRING: %s\n\n", test_string);
			
			//Test the matching
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			//Define a test string -- should fail
			test_string = "aaa  b-b#bbbbabclmnop";
			printf("TEST STRING: %s\n\n", test_string);
			
			//Test the matching
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			//Destroy the regex
			destroy_regex(tester);

			//Break out if we don't fall through
			return;

		//Wildcard character
		case 59:
			printf("Testing wildcard character\n");
			printf("REGEX: 'a(l$a)*cd'\n");

			// Define tester
			tester = define_regular_expression("a(l$a)*cd", REGEX_VERBOSE);

			//Define a test string
			test_string = "adsfaloalaalbacdas";
			printf("TEST STRING: %s\n\n", test_string);
			
			//Test the matching
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			//Define a test string -- should fail
			test_string = "aaa  b-b#bbbbabclmnop";
			printf("TEST STRING: %s\n\n", test_string);
			
			//Test the matching
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			//Destroy the regex
			destroy_regex(tester);

			//Break out if we don't fall through
			return;

		//Wildcard character
		case 60:
			printf("Testing wildcard escape\n");
			printf("REGEX: 'a\\$cd'\n");

			// Define tester
			tester = define_regular_expression("a\\$cd", REGEX_VERBOSE);

			//Define a test string
			test_string = "adsfaloalaalba$cdas";
			printf("TEST STRING: %s\n\n", test_string);
			
			//Test the matching
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			//Define a test string -- should fail
			test_string = "aaa  b-b#bbbbabclmnop";
			printf("TEST STRING: %s\n\n", test_string);
			
			//Test the matching
			regex_match(tester, &matcher, test_string, 0, REGEX_VERBOSE);

			//Destroy the regex
			destroy_regex(tester);

			//Break out if we don't fall through
			return;
	

		//Added to avoid comptime errors, we shouldn't reach this
		default:
			return;
	}
}


/**
 * Runs the whole suite if no argument is passed in
*/
int main(int argc, char** argv){
	u_int32_t argument;

	if(argc == 2 && (sscanf(argv[1], "%u", &argument) > 0)){
		test_case_run(argument);
	}
	
	//Run them all
	if(argc == 1){
		for(u_int8_t i = 0; i < 60; i++){
			test_case_run(i);
		}
	}

}
