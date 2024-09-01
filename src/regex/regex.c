/**
 * Author: Jack Robbins
 * This file contains the implementation for the regex library API defined in 
 * regex.h . Specifically, this file with procedurally generate a state machine that recognizes
 * strings belonging to a regular expression
 */

#include "regex.h" 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "../stack/stack.h"

/**
 * Tokens that we can use for parsing
 */
enum token {
	STAR,
	QUESTION_MARK,
	LBRACKET,
	RBRACKET,
	LPAREN,
	RPAREN,
	CARROT,
	DOLLAR,
	DOT,
	PIPE,
	BACKSLASH,
	LETTER,
	NUMBER,
	DASH,
	UNDERSCORE,
	FORWARDSLASH,
	COLON,
	SEMICOLON,
	EQUALS
};


typedef struct {
	int is_end;
	
} state_t;


/**
 * A struct that contains everything needed for the dfa that defines
 * the regular expression
 */
typedef struct {

} automaton_t;



/**
 * Make sure that the user entered a valid regular expression before we try 
 * to create a DFA with it
 */
static int validate_regular_expression(char* pattern){
	//Just in case
	if(pattern == NULL){
		printf("REGEX ERROR: Invalid pattern entered\n");
		return 0;
	}

	//Grab the pattern length
	u_int32_t length = strlen(pattern);

	//Make sure the length is valid
	if(length == 0){
		printf("REGEX ERROR: Invalid pattern entered\n");
		return 0;
	}

	//Define a stack for paren/bracket matching
	stack_t* stack = create_stack();

	//Copy the pointer so we can iterate without affecting
	char* cursor = pattern;
	char ch;
	//To hold the top of the stack when we pop
	char* top;

	//Iterate over the entire string
	for(u_int16_t i = 0; i < length; i++){
		//Grab the character
		ch = *cursor;

		//Switch on the character
		switch(ch){
			//If we see a parenthesis, we'll have to match
			case '(':
				push(stack, "(");
				break;

			//Same with a bracket
			case '[':
				push(stack, "[");
				break;

			case ')':
				top = (char*)pop(stack);
				if(top[0] != '('){
					printf("REGEX ERROR: Unmatched parenthesis\n");
					return 0;
				}
				break;

			case ']':
				top = (char*)pop(stack);
				if(top[0] != '['){
					printf("REGEX ERROR: Unmatched brakets\n");
					return 0;
				}
				break;
		}


		//Move the stream up by one
		cursor++;
	}


	//If we get here, we know it's good
	return 1;
}


//STUB
regex_t define_regular_expression(char *pattern){
	regex_t regex;

	validate_regular_expression("([])");

	return regex;
}

//STUB
int regex_match(regex_t regex, char *string){
	return 0;
}
