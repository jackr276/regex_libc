/**
 * Author: Jack Robbins
 * This file contains the implementation for the regex library API defined in 
 * regex.h . Specifically, this file with procedurally generate a state machine that recognizes
 * strings belonging to a regular expression
 */

#include "regex.h" 
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
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
	//The default next 
	state_t* next;
	//The optional second next for alternating states 
	state_t* next_opt;
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
	//The start state of the fragment
	state_t* start;
	//The linked list of all arrows or transitions out of the state
	arrow_list_t* arrows;
};


/**
 * Create and return a state
 */
static state_t* create_state(u_int32_t opt, state_t* next, state_t* next_opt){
	//Allocate a state
 	state_t* state = (state_t*)malloc(sizeof(state_t));

	//Assign these values
	state->opt = opt;
	state->next = next;
	state->next_opt = next;

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

				//Switch back to what should be the paren struct pointer that we were working with
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

				//Increment for later on, because this is postfix, we have to add these back on at the end
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

				//We've seen one more regular(non-special) character
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
 * Create a list containing a single arrow to "out" which is NULL. The state out is always uninitialized, which
 * means that we can actually reuse the pointer to it's pointer as an arrowlist as well
 */
static arrow_list_t* singleton_list(state_t** out){
	//Convert to an arrow list, the union type allows us to do this
	arrow_list_t* l = (arrow_list_t*)out;

	//The next pointer is NULL, this hasn't been attached to any other states yet
	l->next = NULL;

	//Return the pointer
	return l;
}


/**
 * Path the list of states contained in the arrow_list out to point to the start state
 * of the next fragement "start"
 */
void concatenate_states(arrow_list_t* out_list, state_t* start){
	//A cursor so we don't affect the original pointer
	arrow_list_t* cursor = out_list;

	//Go over the entire outlist
	while(cursor != NULL){
		//Patch the cursor's state to be start
		cursor->state = start;
		//Advance the cursor
		cursor = cursor->next;
	}
}


/**
 * Connect the two linked lists of list_1 and list_2 to be one big linked list
 * with list_1 as the head
 */
arrow_list_t* concatenate_lists(arrow_list_t* list_1, arrow_list_t* list_2){
	//A cursor for traversal
	arrow_list_t* cursor = list_1;

	//Find the tail of list 1
	while(cursor->next != NULL){
		cursor = cursor->next;
	}

	//Concatenate the lists
	cursor->next = list_2;

	return list_1;
}


/**
 * Build an NFA for a regular expression defined by the pattern
 * passed in.
 *
 * If anything goes wrong, a regex_t struct will be returned in a REGEX_ERR state. This regex
 * will then be useless by the match function
 */
regex_t define_regular_expression(char* pattern){
	//Stack allocate a regex
	regex_t regex;
	//Set to NULL as a flag
	regex.DFA = NULL;

	//Just in case
	if(pattern == NULL || strlen(pattern) == 0){
		printf("REGEX ERROR: Pattern cannot be null or empty\n");
		//Set this flag so that the user knows
		regex.state = REGEX_ERR;
		return regex;
	}

	//Set a hard limit. I don't see a situation where we'd need more than 150 characters for a regex
	if(strlen(pattern) >= 150){
		printf("REGEX ERROR: Patterns of size 150 or more not supported\n");
		regex.state = REGEX_ERR;
		return regex;
	}

	//Convert to postfix before applying our algorithm
	char* postfix = in_to_post(pattern);
	
	//If this didn't work, we will stop and return a bad regex
	if(postfix == NULL){
		printf("REGEX ERROR: Postfix conversion failed.\n");
		//Put in error state
		regex.state = REGEX_ERR;
		return regex;
	}

	//Create a stack for pushing/popping
	stack_t* stack = create_stack();

	//Declare these for use 
	NFA_fragement_t* frag_2;
	NFA_fragement_t* frag_1;
	state_t* split;

	//Keep track of chars processed
	u_int16_t num_processed = 0;

	//Iterate until we hit the null terminator
	for(char* cursor = postfix; *cursor != '\0'; cursor++){
		//Grab the current char
		char ch = *cursor;

		//Switch on the character
		switch(ch){
			//Concatenation character
			case '`':
				//Found one more
				num_processed++;
				
				//Pop the 2 most recent literals off of the stack
				frag_2 = (NFA_fragement_t*)pop(stack);
			    frag_1 = (NFA_fragement_t*)pop(stack);

				//We'll need to map all of the transitions in fragment 1 to point to the start of fragment 2
				concatenate_states(frag_1->arrows, frag_2->start);

				break;

			//Alternate state
			case '|':
				num_processed++;
				//Grab the two most recent fragments off of the stack
				frag_2 = (NFA_fragement_t*)pop(stack);
				frag_1 = (NFA_fragement_t*)pop(stack);

				//Create a new special "split" state that acts as a fork in the road between the two
				//fragment statrt states
				split = create_state(SPLIT, frag_1->start,  frag_2->start);

				//Append the arrow lists of the two fragments so that the new state split has them both
				arrow_list_t* combined = concatenate_lists(frag_1->arrows,frag_2->arrows);

				//Push the newly made state and its transition list onto the stack
				push(stack, create_fragment(split,  combined));

				break;

			//0 or more, more specifically the kleene star
			case '*':
				num_processed++;

				//Pop the most recent fragment
				frag_1 = pop(stack);

				//Create a new state. This new state will act as our split. This state will point to the start of the fragment we just got
				split = create_state(SPLIT, frag_1->start, NULL);

				//Make the arrows in the old fragment point back to the start of the state
				concatenate_states(frag_1->arrows, split);

				//Create a new fragment that originates at the new state, allowing for our "0 or many" function here
				push(stack, create_fragment(split, singleton_list(&(split->next))));

				break;

			//1 or more, more specifically positive closure
			case '+':
				num_processed++;

				//Grab the most recent fragment
				frag_1 = pop(stack);

				//We'll create a new state that acts as a split, going back to the the original state
				//This acts as our optional 0 or 1
				split = create_state(SPLIT, frag_1->start, NULL);

				//Set the most recent fragment to point to this new state so that it's connected
				concatenate_states(frag_1->arrows, split);

				//Create a new fragment that represent this whole structure and push to the stack
				push(stack, create_fragment(frag_1->start, singleton_list(&(split->next))));
			
				break;

			//0 or 1
			case '?':
				num_processed++;

				//Grab the most recent fragment
				frag_1 = pop(stack);

				//We'll create a new state that acts as a split, but this time we won't add any arrows back to this
				//state. This allows for a "zero or one" function
				split = create_state(SPLIT, frag_1->start, NULL);

				//Note how for this one, we won't concatenate states at all

				//Create a new fragment that starts at the split, and represents this whole structure. We also need to chain the lists together to keep everything connected
				push(stack, create_fragment(split, concatenate_lists(frag_1->arrows, singleton_list(&(split->next)))));
				break;

			//Any character that is not one of the special characters
			default:
				//One more processed
				num_processed++;
				//Create a new state with the charcter, and no attached states
				state_t* s = create_state(ch, NULL, NULL);
				//Create a fragment
				NFA_fragement_t* fragment = create_fragment(s,  singleton_list(&(s->next)));

				//Push the fragment onto the stack. We will pop it off when we reach operators
				push(stack,  fragment);
				break;
		}
	}

	//Grab the last fragment off of the stack
	NFA_fragement_t* final = (NFA_fragement_t*)pop(stack);

	//If the stack isn't empty here, it's bad
	if(peek(stack) != NULL){
		printf("REGEX ERROR: Bad regular expression detected.\n");
		//Put in an error state
		regex.state = REGEX_ERR;

		//Cleanup
		free(postfix);
		destroy_stack(stack);

		//Return the regex in an error state
		return regex;
	}

	//Create the accepting state
	state_t* accepting_state = create_state(ACCEPTING, NULL, NULL);

	//Patch in the accepting state
	concatenate_states(final->arrows, accepting_state);

	//This fragment should be the whole DFA, so it's start state should be the start state that we need
	regex.DFA = final->start;
	regex.regex = pattern;
	regex.state = REGEX_VALID;

	//Cleanup
	free(postfix);
	destroy_stack(stack);
	return regex;
}


/**
 *
 * A value of -1 means an invalid input was passed
 */
regex_match_t regex_match(regex_t regex, char* string){
	regex_match_t match;

	//If we are given a bad regex 
	if(regex.state != REGEX_ERR){
		printf("REGEX ERROR: Attempt to use an invalid regex.\n");

		//Pack in the values and return
		match.match = NULL;
		match.match_status = -1;
		return match;
	}

	//If we are given a bad string
	if(string == NULL || strlen(string) == 0){
		printf("REGEX ERROR: Attempt to match a NULL string or a string of length 0.\n");

		//Pack in the values and return
		match.match = NULL;
		match.match_status = -1;
		return match;
	}
	
	return match;
}
