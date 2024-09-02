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

#define ACCEPTING 128
#define SPLIT 127 
#define START 0
#define REGEX_LEN 150
#define CONCATENATION '`'

//For convenience
typedef struct state_t state_t;
typedef struct NFA_fragement_t NFA_fragement_t;
typedef union arrow_list_t arrow_list_t;

/**
 * A struct that defines an NFA state
 * If opt < 256, we have a labeled arrow stored in out
 * If opt = SPLIT, we have two arrows
 * If opt = ACCEPTING, we have an accepting state
 */
struct state_t {
	u_int8_t opt;
	//The default path
	state_t* path;
	//The optional second path
	state_t* path_opt;
};


/**
 * Define a list of the arrows or transitions between
 * states
 */
union arrow_list_t {
	arrow_list_t* next;
	state_t* state;
};


/**
 * A struct that defines an NFA fragement with a list of arrows
 * and a start state for that specific fragment. Remember, a big 
 * NFA can be thought of as a bunch of small ones concatenated, and 
 * that will be our approach here
 */
struct NFA_fragement_t {
	state_t* start;
	arrow_list_t* arrows;
};


/**
 * Create and return a state
 */
static state_t* create_state(u_int32_t opt, state_t* path, state_t* path_opt){
	//Allocate a state
 	state_t* state = (state_t*)malloc(sizeof(state_t));

	//Assign these values
	state->opt = opt;
	state->path = path;
	state->path_opt = path_opt;

	//Give the pointer back
	return state;
}


/**
 * Create and return a fragment
 */
static NFA_fragement_t* create_fragment(state_t* start, arrow_list_t* arrows){
	//Allocate our fragment
	NFA_fragement_t* fragment = (NFA_fragement_t*)malloc(sizeof(NFA_fragement_t));

	fragment->start = start;
	fragment->arrows = arrows;

	//Return a reference to the fragment
	return fragment;
}


/**
 * Convert a regular expression from infix to postfix notation. The character
 * '`' is used as the explicit concatentation operator
 */
static char* in_to_post(char* regex){
	//Allocate space with calloc so we don't have to worry about null termination
	char* postfix = calloc(REGEX_LEN * 2, sizeof(char));
	//Use buffer as our cursor
	char* buffer = postfix;

	//Create a stack for us to use
	stack_t* stack = create_stack();

	//The number of '|' and regular chars that we see when inside parenthesis
	struct paren_alt_reg {
		u_int8_t paren_pipes;
		u_int8_t paren_reg;
	};
	
	//Define an array of paren structs and a pointer to the start of them
	struct paren_alt_reg parens[50];
	struct paren_alt_reg* p = parens;

	//The number of regular characters that we've seen
	u_int8_t num_reg_char = 0;
	//The number of "pipes" we've seen
	u_int8_t num_pipes = 0;

	//A cursor that we have so we don't mess with the original reference
	//Go through every char in the regex
	for(char* cursor = regex; *cursor != '\0'; cursor++){
		//Ensure that we actually have printable chars in here
		if(*cursor < 33 || *cursor > 126){
			printf("REGEX ERROR: Unprintable characters detected.\n");
			return NULL;
		}

		//Switch on the char that we have
		switch(*cursor){
			//Open paren
			case '(':
				//If we see more than 50 nested parenthesis, exit
				if(p >= parens + 50){
					printf("REGEX ERROR: Too many parenthesis in the regex");
					return NULL;
				}

				//Push the open paren onto the stack for matching purposes
				push(stack, "(");
				
				//If we've seen chars, we'll have to concatenate
				if(num_reg_char > 1){
					num_reg_char--; 
					*buffer = CONCATENATION;
					buffer++;
				}

				//Copy the total number of pipes and regular chars 
				p->paren_pipes = num_pipes;
				p->paren_reg = num_reg_char;

				//Next parenthesis struct
				p++;

				//These will be handled by the inside of the parenthesis now, so 0 them out
				num_pipes = 0;
				num_reg_char = 0;

				break;
				
			case ')':
				//Make sure the parenthesis match
				if(*((char*)pop(stack)) != '('){
					printf("REGEX ERROR: Unmatched parenthesis detected.\n");
					return NULL;
				}

				//If this is 0, the user had empty parenthesis
				if(num_reg_char == 0){
					printf("REGEX ERROR: Empty parenthesis detected.\n");
					return NULL;
				}

				//Add in any remaining needed concat operators
				for(; num_reg_char > 1; num_reg_char--){
					*buffer = CONCATENATION;
					buffer++;
				}

				//Add in all of the pipes in postfix order
				for(; num_pipes > 0; num_pipes--){
					*buffer = '|';
					buffer++;
				}

				//Switch back to what should be the paren struct pointer
				p--;

				//Restore these now that the parens are over
				num_reg_char = p->paren_reg;
				num_pipes = p->paren_pipes;

				//Undercounts for some reason
				num_reg_char++;
				break;

			//Alternate operator
			case '|':
				//If we haven't seen any of these, it's bad
				if(num_reg_char == 0){
					printf("REGEX ERROR: Cannot use '|' without two valid operands.\n");
					//Dealloc stack
					destroy_stack(stack);
					//Destroy buffer
					free(buffer);
					return NULL;
				}

				//So long as we've counted some regular characters
				while(num_reg_char > 1){
					//Add a concat operator
					*buffer = CONCATENATION;
					//Move up the cursor
					buffer++;
					num_reg_char--;
				}

				//Increment for later on
				num_pipes++;
				break;

			//O or many, 1 or many, 0 or 1 operators
			case '*':
			case '+':
			case '?':
				//If we've seen no real chars, then these are invalid
				if(num_reg_char == 0){
					printf("REGEX ERROR: Cannot use operator %c without valid operands.\n", *cursor);
					//Dealloc stack
					destroy_stack(stack);
					//Destroy buffer
					free(buffer);
					return NULL;
				}

				//Add these into the postfix
				*buffer = *cursor;
				buffer++;
				break;

			//Anything that isn't a special character is treated normally
			default:
				//If we've already seen more than one regular char, we need to add a concat operator
				if(num_reg_char > 1){
					*buffer = CONCATENATION;
					buffer++;
					num_reg_char--;
				}

				num_reg_char++;
				//Add the character in
				*buffer = *cursor;
				buffer++;
				break;
		}
	}

	//Add in any remaining needed concat operators
	for(; num_reg_char > 1; num_reg_char--){
		*buffer = CONCATENATION;
		buffer++;
	}

	//Add in all of the pipes in postfix order
	for(; num_pipes > 0; num_pipes--){
		*buffer = '|';
		buffer++;
	}

	//Destroy the stack we used here
	destroy_stack(stack);
	//Return the buffer
	return postfix;
}


/**
 * Build an NFA for a regular expression defined by the pattern
 * passed in
 */
regex_t define_regular_expression(char* pattern){
	//Just in case
	if(pattern == NULL || strlen(pattern) == 0){
		printf("REGEX ERROR: Pattern cannot be null or empty\n");
		exit(1);
	}

	//Set a hard limit. I don't see a situation where we'd need more than 150 characters for a regex
	if(strlen(pattern) >= 150){
		printf("REGEX ERROR: Patterns of size 150 or more not supported\n");
		exit(1);
	}

	//Convert to postfix
	char* postfix = in_to_post(pattern);
	printf("%s\n", postfix);

	//Stack allocate a regex
	regex_t regex;
	//Create the start state
	state_t* start = create_state(START, NULL, NULL);
	regex.DFA = start;

	//Grab a copy so we don't mess with the pattern
	char* cursor = pattern;

	//Iterate until we hit the null terminator
	for(; *cursor != '\0'; cursor++){
		//Grab the current char
		char ch = *cursor;

		switch(ch){
			


			//Any literal character
			default:

				break;
				

		}
	}


	return regex;
}

//STUB
int regex_match(regex_t regex, char *string){
	return 0;
}
