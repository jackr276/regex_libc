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
#include <sys/select.h>
#include <sys/types.h>
#include "../stack/stack.h"

#define ACCEPTING 128
#define SPLIT 127 
#define START 0
#define REGEX_LEN 150
#define CONCATENATION '`'

//For convenience
typedef struct NFA_state_t NFA_state_t;
typedef struct NFA_fragement_t NFA_fragement_t;
typedef struct transition_list_t transition_list_t;
typedef struct state_list_t state_list_t ;
typedef struct NFA_state_list_t NFA_state_list_t;
typedef struct DFA_state_t DFA_state_t;

/**
 * A struct that defines an NFA state
 * If opt < 128, we have a labeled arrow stored in out
 * If opt = SPLIT, we have two arrows
 * If opt = ACCEPTING, we have an accepting state
 */
struct NFA_state_t {
	u_int8_t opt;
	//The default next 
	NFA_state_t* next;
	//The optional second next for alternating states 
	NFA_state_t* next_opt;
};


/**
 * Define a list of the transitions between
 * states
 */
struct transition_list_t {
	//The next arrow list struct
	transition_list_t* next;
	//The state that we point to
	NFA_state_t* state;
};


/**
 * A struct that defines an NFA fragement with a list of arrows
 * and a start state for that specific fragment. Remember, a big 
 * NFA can be thought of as a bunch of small ones concatenated, and 
 * that will be our approach here
 */
struct NFA_fragement_t {
	//The start state of the fragment
	NFA_state_t* start;
	//The linked list of all arrows or transitions out of the state
	transition_list_t* arrows;
};


/**
 * When converting from an NFA to a DFA, we represent each DFA state as a list of reachable
 * NFA states. This struct will be used for this purpose
 */
struct NFA_state_list_t {
	NFA_state_t** states;
	u_int16_t length;
};


/**
 * A state that we will use for our DFA caching scheme. It is well known that DFA's are more efficent
 * than NFAs, so this will help us speed things up
 */
struct DFA_state_t {
	//The list of the NFA states that make up the DFA state
	NFA_state_list_t nfa_state_list;
	//This list is a list of all the states that come from this DFA state. We will use the char itself to index this state. Remember that printable
	//chars range from 0-127
	DFA_state_t** transitions;
	DFA_state_t* left;
	DFA_state_t* right;

};


/**
 * Convert a regular expression from infix to postfix notation. The character
 * '`' is used as the explicit concatentation operator
 */
static char* in_to_post(char* regex, regex_mode_t mode){
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
			//Display error if in verbose mode
			if(mode == REGEX_VERBOSE){
				printf("REGEX ERROR: Unprintable characters detected.\n");
			}

			return NULL;
		}

		//Switch on the char that we have
		switch(*cursor){
			//Open paren
			case '(':
				//If we see more than 50 nested parenthesis, exit
				if(p >= parens + 50){
					//Display the error if we're in verbose mode
					if(mode == REGEX_VERBOSE){
						printf("REGEX ERROR: Too many parenthesis in the regex");
					}

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
					//Display the error if we're in verbose mode
					if(mode == REGEX_VERBOSE){
						printf("REGEX ERROR: Unmatched parenthesis detected.\n");
					}

					return NULL;
				}

				//If this is 0, the user had empty parenthesis
				if(num_reg_char == 0){
					//Display the error if requested
					if(mode == REGEX_VERBOSE){
						printf("REGEX ERROR: Empty parenthesis detected.\n");
					}
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
					//Display errors if requested
					if(mode == REGEX_VERBOSE){
						printf("REGEX ERROR: Cannot use '|' without two valid operands.\n");
					}

					//Dealloc stack
					destroy_stack(stack, STATES_ONLY);
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
					//Decrement regular chars seen
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
					//Display errors if requested
					if(mode == REGEX_VERBOSE){
						printf("REGEX ERROR: Cannot use operator %c without valid operands.\n", *cursor);
					}

					//Dealloc stack
					destroy_stack(stack, STATES_ONLY);
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

	//We end up with one extra regular char here(overcounting?) so fix this way
	num_reg_char -= num_pipes;

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
	destroy_stack(stack, STATES_ONLY);
	//Return the buffer
	return postfix;
}


/* ================================================== NFA Methods ================================================ */


/**
 * Create and return a state
 */
static NFA_state_t* create_state(u_int32_t opt, NFA_state_t* next, NFA_state_t* next_opt, u_int16_t* num_states){
	//Allocate a state
 	NFA_state_t* state = (NFA_state_t*)malloc(sizeof(NFA_state_t));

	//Assign these values
	state->opt = opt;
	state->next = next;
	state->next_opt = next_opt;

	//Increment the counter
	(*num_states)++;

	//Give the pointer back
	return state;
}


/**
 * Create and return a fragment. A fragment is a partially built NFA. Our system works by building
 * consecutive fragments on top of previous fragments
 */
static NFA_fragement_t* create_fragment(NFA_state_t* start, transition_list_t* arrows){
	//Allocate our fragment
	NFA_fragement_t* fragment = (NFA_fragement_t*)malloc(sizeof(NFA_fragement_t));

	fragment->start = start;
	fragment->arrows = arrows;

	//Return a reference to the fragment
	return fragment;
}


/**
 * Create a list containing a single arrow to out. This is what makes this a starting list.
 */
static transition_list_t* init_list(NFA_state_t* out){
	//Create a new arrow_list_t
	transition_list_t* list = malloc(sizeof(transition_list_t));

	//Assign the state pointer
	list->state = out;
	//The next pointer is NULL, this hasn't been attached to any other states yet
	list->next = NULL;

	//Return the pointer
	return list;
}


/**
 * Path the list of states contained in the arrow_list out to point to the start state
 * of the next fragement "start"
 */
static void concatenate_states(transition_list_t* out_list, NFA_state_t* start){
	//A cursor so we don't affect the original pointer
	transition_list_t* cursor = out_list;

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
static transition_list_t* concatenate_lists(transition_list_t* list_1, transition_list_t* list_2){
	//A cursor for traversal
	transition_list_t* cursor = list_1;

	//Find the tail of list 1
	while(cursor->next != NULL){
		cursor = cursor->next;
	}

	//Concatenate the lists
	cursor->next = list_2;

	return list_1;
}


/**
 * Destroy the arrow list when we are done using it
 * TODO: on a second thought, do we even need the arrow list? Something to think about
 */
static void destroy_transition_list(transition_list_t* list){
	//Temp for freeing
	transition_list_t* temp;

	//Walk the list
	while(list != NULL){
		//Save list
		temp = list;
		//Advance the pointer
		list = list->next;
		//Free the current transition_list_t
		free(temp);
	}
}


static NFA_state_t* create_NFA(char* postfix, regex_mode_t mode, u_int16_t* num_states){
	//Create a stack for pushing/popping
	stack_t* stack = create_stack();

	//Declare these for use 
	NFA_fragement_t* frag_2;
	NFA_fragement_t* frag_1;
	NFA_state_t* split;

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

				//Push a new fragment up where the start of frag_1 points is the start
				push(stack, create_fragment(frag_1->start, frag_2->arrows));

				//We're done with these now, so we should free them
				free(frag_1);
				free(frag_2);

				break;

			//Alternate state
			case '|':
				num_processed++;
				//Grab the two most recent fragments off of the stack
				frag_2 = (NFA_fragement_t*)pop(stack);
				frag_1 = (NFA_fragement_t*)pop(stack);

				//Check to make sure we actually have stuff to work with here
				if(frag_1 == NULL || frag_2 == NULL){
					//Print out if in verbose mode
					if(mode == REGEX_VERBOSE){
						printf("REGEX_ERR: Alternate operator(|) was passed with 0 or 1 input.\n");
					}
	
					//Return as a warning
					return NULL;
				}

				//Create a new special "split" state that acts as a fork in the road between the two
				//fragment statrt states
				split = create_state(SPLIT, frag_1->start,  frag_2->start, num_states);

				//Append the arrow lists of the two fragments so that the new state split has them both
				transition_list_t* combined = concatenate_lists(frag_1->arrows,frag_2->arrows);

				//Push the newly made state and its transition list onto the stack
				push(stack, create_fragment(split,  combined));

				//Free these pointers as they are no longer needed
				free(frag_1);
				free(frag_2);

				break;

			//0 or more, more specifically the kleene star
			case '*':
				num_processed++;

				//Pop the most recent fragment
				frag_1 = pop(stack);

				//Create a new state. This new state will act as our split. This state will point to the start of the fragment we just got
				split = create_state(SPLIT, frag_1->start, NULL, num_states);

				//Make the arrows in the old fragment point back to the start of the state
				concatenate_states(frag_1->arrows, split);

				//Create a new fragment that originates at the new state, allowing for our "0 or many" function here
				push(stack, create_fragment(split, init_list(split->next)));

				//Free this pointer as it is no longer needed
				free(frag_1);

				break;

			//1 or more, more specifically positive closure
			case '+':
				num_processed++;

				//Grab the most recent fragment
				frag_1 = pop(stack);

				//We'll create a new state that acts as a split, going back to the the original state
				//This acts as our optional 0 or 1
				split = create_state(SPLIT, frag_1->start, NULL, num_states);

				//Set the most recent fragment to point to this new state so that it's connected
				concatenate_states(frag_1->arrows, split);

				//Create a new fragment that represent this whole structure and push to the stack
				push(stack, create_fragment(frag_1->start, init_list(split->next)));
			
				//Free this pointer
				free(frag_1);

				break;

			//0 or 1
			case '?':
				num_processed++;

				//Grab the most recent fragment
				frag_1 = pop(stack);

				//We'll create a new state that acts as a split, but this time we won't add any arrows back to this
				//state. This allows for a "zero or one" function
				split = create_state(SPLIT, frag_1->start, NULL, num_states);

				//Note how for this one, we won't concatenate states at all

				//Create a new fragment that starts at the split, and represents this whole structure. We also need to chain the lists together to keep everything connected
				push(stack, create_fragment(split, concatenate_lists(frag_1->arrows, init_list(split->next))));

				//Free this pointer
				free(frag_1);

				break;

			//Any character that is not one of the special characters
			default:
				//One more processed
				num_processed++;
				//Create a new state with the charcter, and no attached states
				NFA_state_t* s = create_state(ch, NULL, NULL, num_states);
				//Create a fragment
				NFA_fragement_t* fragment = create_fragment(s,  init_list(s->next));

				//Push the fragment onto the stack. We will pop it off when we reach operators
				push(stack,  fragment);
				break;
		}
	}

	//Grab the last fragment off of the stack
	NFA_fragement_t* final = (NFA_fragement_t*)pop(stack);

	//If the stack isn't empty here, it's bad
	if(peek(stack) != NULL){
		//Verbose mode
		if(mode == REGEX_VERBOSE){
			printf("REGEX ERROR: Bad regular expression detected.\n");
		}

		//Cleanup
		destroy_stack(stack, STATES_ONLY);

		//Return the regex in an error state
		return NULL;
	}

	//Create the accepting state
	NFA_state_t* accepting_state = create_state(ACCEPTING, NULL, NULL, num_states);

	//Patch in the accepting state
	concatenate_states(final->arrows, accepting_state);

	/* cleanup */
	//We no longer need the final fragment
	destroy_transition_list(final->arrows);

	//Save this before we free final
	NFA_state_t* starting_state = final->start;
	//Free final
	free(final);

	//Free the postfix string
	destroy_stack(stack, STATES_ONLY);

	//Return a pointer to the final fragments starting state, as this fragment is the NFA
	return starting_state;
}

/* ================================================ End NFA Methods ================================================ */


/* ================================================== DFA Methods ================================================ */

/**
 * Search the list to determine if the accepting state is in here. If the state is in here, that means
 * that it is reachable, and as such we have a match
 */
static u_int8_t contains_accepting_state(NFA_state_list_t* l){
	//Search the array for an accepting state
	for(u_int16_t i = 0; i < l->length; i++){
		//If we find one, return true
		if(l->states[i]->opt == ACCEPTING){
			return 1;
		}
	}

	//If we get here, this list doesn't have one, so return false
	return 0;
}


/**
 * Follow all of the arrows that we have and recursively build a DFA state that is itself
 * a list of all the reachable NFA states
 */
static void get_reachable_rec(NFA_state_list_t* list, NFA_state_t* start){
	//Base case
	if(start == NULL || list == NULL){
		return;
	}

	//We have a split, so follow the split recursively
	if(start->opt == SPLIT){
		//Recursively add these 2 branches as well. This is how we convert from an NFA into a DFA
		get_reachable_rec(list, start->next);
		get_reachable_rec(list, start->next_opt);
	}

	//Add this state to the list of NFA states
	list->states[list->length] = start;
	//Increment the length
	list->length++;
}


/**
 * Initialize a DFA state list by reading all of the transitions of states reachable directly from "start". This new
 * list will be stored in "list". "num_states" is the number of DFA states, which should in theory be the maximum number of 
 * states we could possibly have in one of our DFA state lists
 */
static void get_all_reachable_states(NFA_state_t* start, NFA_state_list_t* state_list, u_int16_t num_states){
	//Allocate list space. At most, all NFA states could be reachable by this state, which is why we calloc with num_states
	state_list->states = (NFA_state_t**)calloc(num_states, sizeof(NFA_state_t*));

	//Currently there's nothing, so we'll set this to 0
	state_list->length = 0;

	//Begin our conversion by converting the start state given here into a DFA state
	get_reachable_rec(state_list, start);
}


/**
 * A constructor that returns a DFA state whose "internals" are the list of NFA states
 * that are reachable at this state. This DFA state is dynamically allocated, and as such will need
 * to be destroyed at some point
 */
static DFA_state_t* create_DFA_state(NFA_state_t* nfa_state, u_int16_t num_states){
	//Allocate a DFA state
	DFA_state_t* dfa_state = (DFA_state_t*)malloc(sizeof(DFA_state_t));

	//Allocate a list of transitions that will tell us what the next state(s) are
	dfa_state->transitions = (DFA_state_t**)calloc(128, sizeof(DFA_state_t*));

	//Get all of the reachable NFA states for that DFA state, this is how we handle splits
	get_all_reachable_states(nfa_state, &(dfa_state->nfa_state_list), num_states);
	
	//Return a pointer to our state
	return dfa_state;
}


/**
 * A recursive helper function for DFA creation
 */
static void create_DFA_rec(DFA_state_t* previous, NFA_state_t* nfa_state, u_int16_t num_states){
	//Base case, we're done here
	if(nfa_state == NULL){
		//"dead end" so to speak
		return;
	}

	//Create the new DFA state from this NFA state
	previous->transitions[nfa_state->opt] = create_DFA_state(nfa_state, num_states);
	//Update the previous pointer
	previous = previous->transitions[nfa_state->opt];

	//Recursively create the next DFA state for opt and next opt
	create_DFA_rec(previous, nfa_state->next, num_states);
	create_DFA_rec(previous, nfa_state->next_opt, num_states);
}


/**
 * Translate an NFA into an equivalent DFA
 */
static DFA_state_t* create_DFA(NFA_state_t* nfa_start, u_int16_t num_states){
	//We'll explicitly create the start state here
	DFA_state_t* dfa_start = (DFA_state_t*)malloc(sizeof(DFA_state_t));
	dfa_start->transitions = (DFA_state_t**)calloc(sizeof(DFA_state_t*), 128);

	//Call the recursive helper method to do the rest for us
	create_DFA_rec(dfa_start, nfa_start, num_states);

	//Return a pointer to the start state
	return dfa_start;
}


/* ================================================ End DFA Methods ================================================ */


/**
 * Build an NFA and then DFA for a regular expression defined by the pattern
 * passed in.
 *
 * If anything goes wrong, a regex_t struct will be returned in a REGEX_ERR state. This regex
 * will then be useless by the match function
 */
regex_t define_regular_expression(char* pattern, regex_mode_t mode){
	//Stack allocate a regex
	regex_t regex;
	//Set to NULL as a flag
	regex.NFA = NULL;

	//Just in case
	if(pattern == NULL || strlen(pattern) == 0){
		//Verbose mode
		if(mode == REGEX_VERBOSE){
			printf("REGEX ERROR: Pattern cannot be null or empty\n");
		}

		//Set this flag so that the user knows
		regex.state = REGEX_ERR;
		return regex;
	}

	//Set a hard limit. I don't see a situation where we'd need more than 150 characters for a regex
	if(strlen(pattern) >= REGEX_LEN){
		//Verbose mode
		if(mode == REGEX_VERBOSE){
			printf("REGEX ERROR: Patterns of size 150 or more not supported\n");
		}

		regex.state = REGEX_ERR;
		return regex;
	}

	//Convert to postfix before applying our algorithm
	char* postfix = in_to_post(pattern, mode);

	
	//If this didn't work, we will stop and return a bad regex
	if(postfix == NULL){
		//Verbose mode
		if(mode == REGEX_VERBOSE){
			printf("REGEX ERROR: Postfix conversion failed.\n");
		}

		//Put in error state
		regex.state = REGEX_ERR;
		return regex;
	}

	//Show postfix if it exists
	if(mode == REGEX_VERBOSE){
		printf("Postfix conversion: %s\n", postfix);
	}

	//Initially 0
	regex.num_states = 0;

	//Create the NFA first
	regex.NFA = create_NFA(postfix, mode, &(regex.num_states));

	//If this is bad, we'll bail out here
	if(regex.NFA == NULL){
		if(mode == REGEX_VERBOSE){
			printf("REGEX ERROR: NFA creation failed.\n");
			//Put in an error state
			regex.state = REGEX_ERR;
			
			//Ensure there is no leakage
			free(postfix);

			return regex;
		}
	}

	//Now we'll use the NFA to create the DFA. We'll do this because DFA's are much more
	//efficient to simulate since they are determinsitic, but they are much harder to create
	//from regular expressions
	regex.DFA = create_DFA(regex.NFA, mode);

	if(regex.DFA == NULL){
		if(mode == REGEX_VERBOSE){
			printf("REGEX ERROR: DFA creation failed.\n");
			regex.state = REGEX_ERR;
			free(postfix);
			return regex;
		}
	}


	return regex;
}


/**
 * Error handling: 
 */
regex_match_t regex_match(regex_t regex, char* string, regex_mode_t mode){
	regex_match_t match;
	//Error mode by default
	match.status = MATCH_ERR;

	//If we are given a bad regex 
	if(regex.state != REGEX_ERR){
		//Verbose mode
		if(mode == REGEX_VERBOSE){
			printf("REGEX ERROR: Attempt to use an invalid regex.\n");
		}

		//Pack in the values and return
		match.match_start = 0;
		match.match_end = 0;

		//We return this value so that the caller can know what the error was
		match.status = MATCH_INV_INPUT;

		//Give the regex back
		return match;
	}

	//If we are given a bad string
	if(string == NULL || strlen(string) == 0){
		//Verbose mode
		if(mode == REGEX_VERBOSE){
			printf("REGEX ERROR: Attempt to match a NULL string or a string of length 0.\n");
		}

		//Pack in the values and return
		match.match_start = 0;
		match.match_end = 0;
		match.status = MATCH_INV_INPUT;
		return match;
	}

	//Return the match struct
	return match;
}


/* ================================================== Cleanup ================================================ */


/**
 * Recursively free all NFA states in that are pointed to. We should have no dangling states, so 
 * in theory, this should work
 */
static void teardown_NFA_state(NFA_state_t* state){
	//Base case
	if(state == NULL){
		return;
	}

	//Recursively call free on the next states here
	teardown_NFA_state(state->next);
	teardown_NFA_state(state->next_opt);

	//Free the pointer to this state
	free(state);
}


/**
 * Recursively free all DFA states that are pointed to. We should have no dangling states, so in theory,
 * this should work
 */
static void teardown_DFA_state(DFA_state_t* state){
	//Base case
	if(state == NULL || state->transitions == NULL){
		return;
	}

	//Recursively teardown every other state
	for(u_int16_t i = 0; i < 128; i++){
		teardown_DFA_state(state->transitions[i]);
	}

	//Free the nfa list
	free(state->nfa_state_list.states);

	//Free the transitions array
	free(state->transitions);

	//Free the state overall
	free(state);
}

/**
 * Comprehensive cleanup function that cleans up everything related to the regex
 */
void destroy_regex(regex_t regex){
	//Call the recursive NFA freeing function
	teardown_NFA_state(regex.NFA);
	teardown_DFA_state(regex.DFA);
}


/* ================================================== Cleanup ================================================ */
