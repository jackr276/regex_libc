/**
 * Author: Jack Robbins
 * This file contains the implementation for the regex library API defined in 
 * regex.h . Specifically, this file with procedurally generate a state machine that recognizes
 * strings belonging to a regular expression
 */

#include "regex.h" 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

//Forward declare
typedef struct NFA_state_t NFA_state_t;
typedef struct NFA_fragement_t NFA_fragement_t;
typedef struct fringe_states_t fringe_states_t;
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
	//Was this state visited?
	//0 - default
	//1 - visited by dfa constructor
	//3 - from split
	u_int8_t visited;
	//The char that we hold
	u_int16_t opt;
	//The default next 
	NFA_state_t* next;
	//The optional second next for alternating states 
	NFA_state_t* next_opt;
	//The next created state in the linked list
	NFA_state_t* next_created;
};


/**
 * Define a linked list structure that holds all of the states on the 
 * fringe of a fragment. States on the fringe of a fragment are the ones
 * that will need to be patched into the next fragment, so we should keep track
 * of these
 */
struct fringe_states_t {
	//The next arrow list struct
	fringe_states_t* next;
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
	//Keep track of all of the states on the very edge of our fragment
	fringe_states_t* fringe_states;
};


/**
 * When converting from an NFA to a DFA, we represent each DFA state as a list of reachable
 * NFA states. This struct will be used for this purpose
 */
struct NFA_state_list_t {
	NFA_state_t* states[140];
	u_int16_t length;
	//Does this list contain an accepting state?
	u_int8_t contains_accepting_state;
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
	DFA_state_t* transitions[140];
	//The next dfa_state that was made, this will help us in freeing
	DFA_state_t* next;
};


/**
 * Convert a regular expression from infix to postfix notation. The character
 * '`' is used as the explicit concatentation operator
 *
 * TODO
 * 	Idea for making this recursive to better handle the parenthesis. Right now parenthesis
 * 	simply do not work with | because of the way concatenation is handled
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

			//Explicit escape character
			case '~':
				//Fail case: if the cursor is at the end, we have an issue
				if(*(cursor + 1) == '\0'){
					if(mode == REGEX_VERBOSE){
						printf("REGEX_ERROR: Use of escape character '~' with nothing following it.\n");
					}
					//Return NULL since this would be a fail case
					free(buffer);
					return NULL;
				}

				//Add in explicit concatenation is we need to
				if(num_reg_char > 1){
					*buffer = CONCATENATION;
					buffer++;
					num_reg_char--;
				}

				//We've seen one more regular character
				num_reg_char++;

				//Advance the cursor to skip over this char 
				cursor++;

				//Add this into the buffer
				*buffer = '~';
				//Push up the buffer pointer
				buffer++;

				//Add this character into the buffer
				*buffer = *cursor;
				
				//Advance the buffer
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
	state->visited = 0;
	state->opt = opt;
	state->next = next;
	state->next_opt = next_opt;
	//Must be set later on
	state->next_created = NULL;

	//Increment the counter
	(*num_states)++;

	//Give the pointer back
	return state;
}


/**
 * Create and return a fragment. A fragment is a partially built NFA. Our system works by building
 * consecutive fragments on top of previous fragments
 */
static NFA_fragement_t* create_fragment(NFA_state_t* start, fringe_states_t* fringe_states){
	//Allocate our fragment
	NFA_fragement_t* fragment = (NFA_fragement_t*)malloc(sizeof(NFA_fragement_t));

	fragment->start = start;
	fragment->fringe_states = fringe_states;

	//Return a reference to the fragment
	return fragment;
}


/**
 * Create a list of fringe states containing just the singular state. This will be used
 * when we first create a new fragment for a single character, since when this happens, the
 * only thing in the "fringe" is that fragment itself
 */
static fringe_states_t* init_list(NFA_state_t* state){
	//Create a new fringe_states_t 
	fringe_states_t* list = malloc(sizeof(fringe_states_t));

	//Assign the state pointer
	list->state = state;
	//The next pointer is NULL, this hasn't been attached to any other states yet
	list->next = NULL;

	//Return the pointer
	return list;
}


/**
 * Make the list of states contained in the arrow_list out to point to the start state
 * of the next fragement "start"
 *
 * point_opt will make use of next if 1, next_opt if 0
 * NOTE: we really should never use next_opt UNLESS we are a split state, in which case we never even
 * get here
 */
static void concatenate_states(fringe_states_t* fringe, NFA_state_t* start, u_int8_t point_opt){
	//A cursor so we don't affect the original pointer
	fringe_states_t* cursor = fringe;

	//Go over the entire outlist
	while(cursor != NULL){
		//If this has a valid state
		if(cursor->state != NULL){
			//This fringe states next state should be the new start state
			if(point_opt == 1){
				cursor->state->next = start;
			} else {
				cursor->state->next_opt = start;
			}
		}

		//Advance the cursor
		cursor = cursor->next;
	}
}


/**
 * Connect the two linked lists of list_1 and list_2 to be one big linked list
 * with list_1 as the head
 */
static fringe_states_t* concatenate_lists(fringe_states_t* list_1, fringe_states_t* list_2){
	//A cursor for traversal
	fringe_states_t* cursor = list_1;

	//Find the tail of list 1
	while(cursor->next != NULL){
		cursor = cursor->next;
	}

	//Concatenate the lists
	cursor->next = list_2;

	return list_1;
}


/**
 * Destroy the list of fringe states once we are done using it to avoid any
 * memory leakage
 */
static void destroy_fringe_list(fringe_states_t* list){
	//Temp for freeing
	fringe_states_t* temp;

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


/**
 * Print out a list of free states for debugging/verbose purposes
 */
static void print_fringe_states(fringe_states_t* list){
	fringe_states_t* cursor = list;

	printf("All states currently in fringe: ");

	while(cursor != NULL){
		printf("%c", cursor->state->opt);
		cursor = cursor->next;
	}

	printf("\n");

}


/**
 * Ability to print out an NFA for debug purposes
 */
static void print_NFA(NFA_state_t* nfa){
	if(nfa == NULL || nfa->visited == 2){
		return;
	}

	if(nfa->opt != ACCEPTING){
		nfa->visited = 2;
	}

	//Support printing of special characters split and accepting
	if(nfa->opt == SPLIT_ALTERNATE){
		printf("State -SPLIT_ALTERNATE->");
 	} else if(nfa->opt == SPLIT_ZERO_OR_ONE){
		printf("State -SPLIT_ZERO_OR_ONE->");
	} else if(nfa->opt == SPLIT_POSITIVE_CLOSURE){
		printf("State -SPLIT_POSITIVE_CLOSURE->");
	} else if(nfa->opt == SPLIT_KLEENE){
		printf("State -SPLIT_KLEENE->");
	} else if(nfa->opt == ACCEPTING){
		printf("State -ACCEPTING->");
	} else {
		printf("State -%c->", (u_int8_t)nfa->opt);
	}

	if(nfa->opt == SPLIT_ALTERNATE || nfa->opt == SPLIT_ZERO_OR_ONE){
		print_NFA(nfa->next);
		printf("\n");
		print_NFA(nfa->next_opt);
	} else if(nfa->opt == SPLIT_KLEENE || nfa->opt == SPLIT_POSITIVE_CLOSURE){
		print_NFA(nfa->next);
		printf("\n");
		print_NFA(nfa->next_opt);
		nfa->visited = 2;
	} else {
		print_NFA(nfa->next);
	}
}


/**
 * Ability to print out a DFA for debug purposes
 * This method does not necessarily work for printing out all of the DFA states in 
 * the way that you might expect
 */
static void print_DFA(DFA_state_t* dfa){
	//If the state is null we'll stop
	if(dfa == NULL){
		printf("DFA was not initialized");
		return;
	}

	//Grab a cursor to our DFA
	DFA_state_t* cursor = dfa;
	u_int16_t i = 0;

	while(cursor != NULL){
		printf("State %d, internal states: {", i);
		for(u_int16_t i = 0; i < cursor->nfa_state_list.length && cursor->nfa_state_list.states[i] != NULL; i++){
			u_int16_t opt = cursor->nfa_state_list.states[i]->opt;
			//Printing internals here
			i == ACCEPTING ? printf("ACCEPTING, ") : printf("%c, ", opt);
		}

		//Print all of the states that we are able to reach from this state
		printf("}, reachable: {");
		for(u_int16_t i = 0; i < 140; i++){
			if(cursor->transitions[i] != NULL){
				i == ACCEPTING ? printf("ACCEPTING, ") : printf("%c, ", i);
			}
		}

		printf("} -> \n");

		//Advance the cursor
		cursor = cursor->next;
		i++;
	}
}


/**
 * Create a new state in memory that is completely identical to "state"
 */
static NFA_state_t* copy_state(NFA_state_t* state){
	//Create a new state
	NFA_state_t* copy = (NFA_state_t*)malloc(sizeof(NFA_state_t));

	//Perform a deep copy
	copy->visited = 0;
	copy->next = state->next;
	copy->opt = state->opt;
	copy->next_opt = state->next_opt;
	copy->next_created = state->next_created;
	
	return copy;
}


/**
 * Create a deep copy of a fragment that is totally independent from
 * the predecessor in memory
 */
static NFA_fragement_t* copy_fragment(NFA_fragement_t* frag){
	//Create the fragment copy
	NFA_fragement_t* copy = (NFA_fragement_t*)malloc(sizeof(NFA_fragement_t));

	//Copy the start state
	copy->start = copy_state(frag->start);

	//Grab a cursor to iterate through
	fringe_states_t* cursor = frag->fringe_states;
	fringe_states_t* current_fringe_state = copy->fringe_states;

	while(cursor != NULL){
		current_fringe_state = malloc(sizeof(fringe_states_t));
		current_fringe_state->state = cursor->state;
		current_fringe_state->next = cursor->next;

		cursor = cursor->next;
	}

	return copy;
}

/**
 * Create an NFA from a postfix regular expression FIXME does not work for () combined with *, | or +
 */
static NFA_state_t* create_NFA(char* postfix, regex_mode_t mode, u_int16_t* num_states){
	//Create a stack for pushing/popping
	stack_t* stack = create_stack();
	//A linked list for us to hold all of our created states
	//The very end of our linked list
	NFA_state_t* tail =  NULL;

	//Declare these for use 
	NFA_fragement_t* frag_2;
	NFA_fragement_t* frag_1;
	NFA_fragement_t* fragment;
	NFA_state_t* split;
	NFA_state_t* s;

	//Declare this for our use as well
	fringe_states_t* fringe;

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

				//We need to set all of the fringe states in fragment 1 to point to the start
				//of fragment 2
				//Concatenation ALWAYS follows the "next" option without exception
				concatenate_states(frag_1->fringe_states, frag_2->start, 1);

				//The fringe list of fragment 1 should be irrelevant now, so we can get rid of it
				destroy_fringe_list(frag_1->fringe_states);

				//Push a new fragment up where the start of frag_1 points is the start, and all of the fringe states
				//are fragment 2's fringe states
				push(stack, create_fragment(frag_1->start, frag_2->fringe_states));

				//However, we do still need the states in frag_2->fringe_states, so we'll leave those be	

				//If we're in verbose mode, alert that a fragment was made
				if(mode == REGEX_VERBOSE){
					printf("\nFragment created concatenating characters %c and %c\n", frag_2->start->opt, frag_1->start->opt);
				}	

				//We're done with these now, so we should free them
				free(frag_1);
				free(frag_2);

				break;

			//Alternate state
			case '|':
				//Grab the two most recent fragments off of the stack
				frag_2 = (NFA_fragement_t*)pop(stack);
				frag_1 = (NFA_fragement_t*)pop(stack);

				if(frag_2 == NULL || frag_1 == NULL) printf("NULL ENCOUNTERED");
				//Create a new special "split" state that acts as a fork in the road between the two
				//fragment start states
				//This is leaking memory
				split = create_state(SPLIT_ALTERNATE, frag_1->start,  frag_2->start, num_states);

				//If this is the very first state then it is our origin for the linked list
				if(num_processed == 0){
					tail = split;
				} else {
					//Add onto linked list
					tail->next_created = split;
					tail = split;
				}

				num_processed++;

				//Combine the two fringe lists to get the new list of all fringe states for this fragment
				fringe_states_t* combined = concatenate_lists(frag_1->fringe_states, frag_2->fringe_states);

				//Push the newly made state and its transition list onto the stack
				push(stack, create_fragment(split,  combined));

				//Notice how we won't free any lists here, because they both still hold data about the fringe
				//Free these pointers as they are no longer needed
				free(frag_1);
				free(frag_2);

				break;

			//0 or more, more specifically the kleene star
			case '*':
				//Pop the most recent fragment
				frag_1 = pop(stack);

				//Create a new state. This new state will act as our split. This state will point to the start of the fragment we just got
				split = create_state(SPLIT_KLEENE, NULL, frag_1->start, num_states);
				
				//If this is the very first state then it is our origin for the linked list
				if(num_processed == 0){
					tail = split;
				} else {
					//Add onto linked list
					tail->next_created = split;
					tail = split;
				}

				num_processed++;

				//Print out the fringe states DEBUGGING STATEMENT
				if(mode == REGEX_VERBOSE){
					print_fringe_states(frag_1->fringe_states);
				}

				//Make all of the states in fragment_1 point to the beginning of the split 
				//using their next_opt to allow for our "0 or more" functionality 
				concatenate_states(frag_1->fringe_states, split, 1);

				//Create a new fragment that originates at the new state, allowing for our "0 or many" function here
				push(stack, create_fragment(split, init_list(split)));

				//Free this pointer as it is no longer needed
				free(frag_1);

				break;

			//1 or more, more specifically positive closure
			case '+':
				//Grab the most recent fragment
				frag_1 = pop(stack);
				frag_2 = copy_fragment(frag_1);

				//We'll create a new state that acts as a split, going back to the the original state
				//This acts as our optional 1 or more 
				split = create_state(SPLIT_POSITIVE_CLOSURE, NULL, frag_2->start, num_states);
	
				//If this is the very first state then it is our origin for the linked list
				if(num_processed == 0){
					tail = split;
				} else {
					//Add onto linked list
					tail->next_created = split;
					tail = split;
				}

				num_processed++;

				//Print out the fringe states DEBUGGING STATEMENT
				if(mode == REGEX_VERBOSE){
					print_fringe_states(frag_1->fringe_states);
				}

				//Set all of the fringe states in frag_1 to point at the split
				concatenate_states(frag_1->fringe_states, split, 1);

				//Create a new fragment that represent this whole structure and push to the stack
				//Since this one is "1 or more", we will have the start of our next fragment be the start of the old fragment
				//THIS is the problem here, we can't have this guy point to fragment->start. It has to point to the immediately preceeding
				//state
				//push(stack, create_fragment(frag_1->start, concatenate_lists(frag_1->fringe_states, init_list(split))));
				push(stack, create_fragment(frag_1->start, init_list(split)));

				//Free this pointer
				free(frag_1);

				break;

			//0 or 1 instances of the preceding fragment
			case '?':
				//Grab the most recent fragment
				frag_1 = pop(stack);

				//We'll create a new state that acts as a split, but this time we won't add any arrows back to this
				//state. This allows for a "zero or one" function
				//NOTE: Here, we'll use Split's next-opt to point back to the fragment at the start
				split = create_state(SPLIT_ZERO_OR_ONE, NULL, frag_1->start, num_states);

				//If this is the very first state then it is our origin for the linked list
				if(num_processed == 0){
					tail = split;
				//We'll need to insert this into the linked list in the right position
				} else {
					//Add onto linked list
					tail->next_created = split;
					tail = split;
				}

				num_processed++;

				//Note how for this one, we won't concatenate states at all, but we'll instead concatentate
				//the two fringe lists into one big one because the fringe is a combined fringe
				fringe = concatenate_lists(frag_1->fringe_states, init_list(split));
				
				//Create a new fragment that starts at the split, and represents this whole structure. We also need to chain the lists together to keep everything connected
				push(stack, create_fragment(split, fringe));

				//We won't free the fragments list here because it still holds fringe data
				//Free this pointer
				free(frag_1);

				break;

			//If we see the escape character, then we process the immediately next character as a regular char
			case '~':
				cursor++;

				//Create a new state with the escaped character
				s = create_state(*cursor, NULL,  NULL,  num_states);

				if(num_processed == 0){
					tail = s;
				} else {
					tail->next_created = s;
					tail = s;
				}

				//Create a fragment with the fringe states being the new state that we created
				fragment = create_fragment(s, init_list(s));

				//Push this new fragment to the stack
				push(stack, fragment);

				break;

			//Any character that is not one of the special characters
			default:
				//Create a new state with the charcter, and no attached states
				s = create_state(ch, NULL, NULL, num_states);

				//concatenate to linkedlist
				if(num_processed == 0){
					printf("Making start state\n");
					tail = s;
				} else {
					tail->next_created = s;
					tail = s;
				}

				//One more processed
				num_processed++;

				//Create a fragment, with the fringe states of that fragment being just this new state that we
				//created
				fragment = create_fragment(s,  init_list(s));

				//Push the fragment onto the stack. We will pop it off when we reach operators
				push(stack,  fragment);

				//If we're in verbose mode, print out which character we processed
				if(mode == REGEX_VERBOSE){
					printf("\nAdded fragment for character: %c\n", ch);
				}
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
	//Add into the linked list
	tail->next_created = accepting_state;

	//Set everything in the final fringe to point to the accepting state
	concatenate_states(final->fringe_states, accepting_state, 1);

	/* cleanup */
	//We no longer need the final fragment
	destroy_fringe_list(final->fringe_states);

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


/* ================================================== DFA Methods ================================================== */


/**
 * Follow all of the arrows that we have and recursively build a DFA state that is itself
 * a list of all the reachable NFA states
 */
static void get_reachable_rec(NFA_state_t* start, NFA_state_list_t* list){
	//Base case
	if(start == NULL || list == NULL){
		return;
	}
	//We can tell what to do based on our opt here
	switch(start->opt){
			case SPLIT_KLEENE:
			case SPLIT_POSITIVE_CLOSURE:
			//If we have a split_T2, we know that this state will always point back to
			//itself along the next_opt line
			//We'll only explore the next path, we've already accounted for the self reference
			//Add this state to the list of NFA states
			get_reachable_rec(start->next, list);
			break;
		default:
			//Add this state to the list of NFA states
			list->states[list->length] = start;

			//Increment the length
			list->length++;
			break;
	}

	//If we find an accepting state, then set this flag. This will speed up our match function
	if(start->opt == ACCEPTING){
		list->contains_accepting_state = 1;
	}
}


/**
 * Initialize a DFA state list by reading all of the transitions of states reachable directly from "start". This new
 * list will be stored in "list". "num_states" is the number of DFA states, which should in theory be the maximum number of 
 * states we could possibly have in one of our DFA state lists
 */
static void get_all_reachable_states(NFA_state_t* start, NFA_state_list_t* state_list){
	//Currently there's nothing, so we'll set this to 0
	state_list->length = 0;

	//By default, we don't have an accepting state in our list
	state_list->contains_accepting_state = 0;

	//Begin our conversion by converting the start state given here into a DFA state
	get_reachable_rec(start, state_list);
}


/**
 * A constructor that returns a DFA state whose "internals" are the list of NFA states
 * that are reachable at this state. This DFA state is dynamically allocated, and as such will need
 * to be destroyed at some point
 */
static DFA_state_t* create_DFA_state(NFA_state_t* nfa_state){
	//Allocate a DFA state
	DFA_state_t* dfa_state = (DFA_state_t*)calloc(1, sizeof(DFA_state_t));

	//Set to null as warning
	dfa_state->next = NULL;

	//Legacy - no longer needed with calloc
	//0 out the entire array of DFA transitions as well
//	memset(dfa_state->transitions, 0, 130*sizeof(DFA_state_t*));

	//0 out the entire array of NFA states
//	memset(dfa_state->nfa_state_list.states, 0, 130*sizeof(NFA_state_t*));

	//Get all of the reachable NFA states for that DFA state, this is how we handle splits
	if(nfa_state != NULL){
		get_all_reachable_states(nfa_state, &(dfa_state->nfa_state_list));
	}

	//Return a pointer to our state
	return dfa_state;
}


/**
 * Translate an NFA into an equivalent DFA using the reachability matrix
 * method. We will recursively figure out which states are reachable from other states. Our
 * new linked list of states should help us with this
 */
static DFA_state_t* create_DFA(NFA_state_t* nfa_start, regex_mode_t mode, u_int16_t flag_states){
	//The starting state for our DFA
	DFA_state_t* previous;
	DFA_state_t* temp;
	DFA_state_t* left_opt;
	DFA_state_t* left_opt_mem;
	DFA_state_t* right_opt;
	DFA_state_t* right_opt_mem;

	//The cursor that we may or may not use
	DFA_state_t* cursor;

	//A dummy start state to enter into
	DFA_state_t* dfa_start = create_DFA_state(NULL);

	//Advance previous
	previous = dfa_start;	

	//Maintain a cursor to the current NFA state
	NFA_state_t* nfa_cursor = nfa_start;

	//Iterate through every NFA state. We have a 1-1 nfa-state dfa-state translation
	while(nfa_cursor != NULL){
		//Reset these here
		left_opt_mem = NULL;
		right_opt_mem = NULL;

		//If we've already gotten to this guy from a split, we'll move right on
		if(nfa_cursor->visited == 3 /*&& nfa_cursor->opt != ACCEPTING*/){
			nfa_cursor = nfa_cursor->next;
			continue;
		}

		//We have different processing rules for each of our special cases. The default is of course that we just have a char
		switch(nfa_cursor->opt){
			case SPLIT_ZERO_OR_ONE:
				nfa_cursor->visited = 3;
				//Create the DFA for the straight path, marking each state as "off limits" whenever we see it.
				//This marking of off limits will ensure that the next function call will not retrace this
				//one's steps
				left_opt = create_DFA(nfa_cursor->next, mode, 1);

				//Create the right sub-DFA that is the optional "0 or 1" DFA
				right_opt = create_DFA(nfa_cursor->next_opt, mode, 1);

				//Save these for later for memory deletion
				left_opt_mem = left_opt;
				right_opt_mem = right_opt;

				//Advance these up(remember, we have "dummy heads")
				left_opt = left_opt->next;
				right_opt = right_opt->next;

				/**
				 * We now have
				 * 	The "previous" state, which is what came before the DFA
				 * 	Left_opt: the entire DFA that follows the nonoptional path
				 * 	Right_opt: the entire optional path that represents our "0 or 1"
				 *
				 * 	We'll now do 2 things:
				 * 		Patch in "previous" to point to both left_opt and right_opt
				 * 		patch in the end of right_opt to point to left_opt
				 */

				//Make everything in previous reference point to left_opt
				for(u_int16_t i = 0; i < left_opt->nfa_state_list.length; i++){
					u_int16_t opt = left_opt->nfa_state_list.states[i]->opt;
					previous->transitions[opt] = left_opt;
				}

				//Make everything in previous point to right_opt
				for(u_int16_t i = 0; i < right_opt->nfa_state_list.length; i++){
					u_int16_t opt = right_opt->nfa_state_list.states[i]->opt;
					previous->transitions[opt] = right_opt;
				}

				//Now we'll need to ensure that right_opt(At the very end) points to left_opt

				//Let's first advance to the very end of right_opt
				cursor = right_opt;
				//Advance to very end
				while(cursor->next != NULL){
					cursor = cursor->next;
				}

				//Now cursor has the end of right_opt, so let's make it point to left_opt
				for(u_int16_t i = 0; i < left_opt->nfa_state_list.length; i++){
					u_int16_t opt = left_opt->nfa_state_list.states[i]->opt;
					cursor->transitions[opt] = left_opt;
				}

				//We need to chain all of these together for the eventual memory freeing
				previous->next = left_opt_mem;
				previous = left_opt_mem;
				while(previous->next != NULL){
					previous = previous->next;
				}

				//We need to chain all of these together for the eventual memory freeing
				previous->next = right_opt_mem;
				previous = right_opt_mem;
				while(previous->next != NULL){
					previous = previous->next;
				}

				//In theory the entire thing should now be done so
				return dfa_start;

			//Handle an alternate split
			case SPLIT_ALTERNATE:	
				nfa_cursor->visited = 3;
				//Create two separate sub-DFAs
				//TODO this may be causing issues by not marking stuff. It only exists this way currently to 
				//avoid the accepting state being overlooked
				left_opt = create_DFA(nfa_cursor->next, mode, 0);
				right_opt = create_DFA(nfa_cursor->next_opt, mode, 1);

				//Save these for later
				left_opt_mem = left_opt;
				right_opt_mem = right_opt;

				//Advance past the dummy head
				left_opt = left_opt->next;
				right_opt = right_opt->next;

				/**
				 * To patch this DFA in, we'll make everything in previous point to both the left
				 * and right sub-DFAs. Unlike with a 0 or 1 split, these two separate DFAs should
				 * never point to one another
				 */
				
				//Patching in left_opt
				for(u_int16_t i = 0; i < left_opt->nfa_state_list.length; i++){
					//Grab the char
					u_int16_t opt = left_opt->nfa_state_list.states[i]->opt;
					previous->transitions[opt] = left_opt;
				}

				//Patching in right_opt
				for(u_int16_t i = 0; i < right_opt->nfa_state_list.length; i++){
					//Grab the char
					u_int16_t opt = right_opt->nfa_state_list.states[i]->opt;
					previous->transitions[opt] = right_opt;
				}

				//We need to chain all of these together for the eventual memory freeing
				previous->next = left_opt_mem;
				previous = left_opt_mem;
				while(previous->next != NULL){
					previous = previous->next;
				}

				//We need to chain all of these together for the eventual memory freeing
				previous->next = right_opt_mem;
				previous = right_opt_mem;
				while(previous->next != NULL){
					previous = previous->next;
				}

				//Get out
				return dfa_start;

			//Handle the "0 or more" case, better known as Kleene star
			case SPLIT_KLEENE:
				//Mark as seen before we go any further
				nfa_cursor->visited = 3;

				//Create the entire left sub_dfa(this one does not repeat)
				left_opt = create_DFA(nfa_cursor->next, mode, 1);
				//Here is our actual "repeater"
				right_opt = create_DFA(nfa_cursor->next_opt, mode, 1);


				//Save these for later
				left_opt_mem = left_opt;
				right_opt_mem = right_opt;

				//Advance these so that we actually have them
				left_opt = left_opt->next;
				right_opt = right_opt->next;

				/**
				 * We'll now patch in the "left_opt" such that previous points to it. We'll
				 * then patch in right opt as well. Finally, we'll make it so that right_opt
				 * points back to its own beginning
				 */
				
				//Patching in left_opt
				for(u_int16_t i = 0; i < left_opt->nfa_state_list.length; i++){
					//Grab the char
					u_int16_t opt = left_opt->nfa_state_list.states[i]->opt;
					previous->transitions[opt] = left_opt;
				}
					
				//Patching in right_opt
				for(u_int16_t i = 0; i < right_opt->nfa_state_list.length; i++){
					//Grab the char
					u_int16_t opt = right_opt->nfa_state_list.states[i]->opt;
					previous->transitions[opt] = right_opt;
				}


				//We'll now need to navigate to the end of the right opt repeater
				//sub-DFA
				cursor = right_opt;

				
				while(cursor->next != NULL){
					cursor = cursor->next;
				}

				//Now that we're here, cursor holds the very end of the right sub-DFA
				//Everything that we have in the cursor must point back to the left DFA
				for(u_int16_t i = 0; i < left_opt->nfa_state_list.length; i++){
					//Grab the char
					u_int16_t opt = left_opt->nfa_state_list.states[i]->opt;
					cursor->transitions[opt] = left_opt;
				}
					
				//Patching in right_opt
				//Everything that we have in the cursor must also point back to the right DFA
				for(u_int16_t i = 0; i < right_opt->nfa_state_list.length; i++){
					//Grab the char
					u_int16_t opt = right_opt->nfa_state_list.states[i]->opt;
					cursor->transitions[opt] = right_opt;
				}
			
				//We need to chain all of these together for the eventual memory freeing
				previous->next = left_opt_mem;
				previous = left_opt_mem;
				while(previous->next != NULL){
					previous = previous->next;
				}

				//We need to chain all of these together for the eventual memory freeing
				previous->next = right_opt_mem;
				previous = right_opt_mem;
				while(previous->next != NULL){
					previous = previous->next;
				}

				//Get out
				return dfa_start;

			//Handle the "1 or more" case
			case SPLIT_POSITIVE_CLOSURE:
				//Avoid an infinite loop
				nfa_cursor->visited = 3;
				//Create the left DFA
				left_opt = create_DFA(nfa_cursor->next, mode, 1);
				//Create the right DFA
				right_opt = create_DFA(nfa_cursor->next_opt, mode, 1);
				

				//Save these for later
				left_opt_mem = left_opt;
				right_opt_mem = right_opt;

				//Advance these so that we actually have them
				left_opt = left_opt->next;
				right_opt = right_opt->next;

				/**
				 * We'll now patch in the "left_opt" such that previous points to it. We'll
				 * then patch in right opt as well. Finally, we'll make it so that right_opt
				 * points back to its own beginning
				 */
				
				//Patching in left_opt
				for(u_int16_t i = 0; i < left_opt->nfa_state_list.length; i++){
					//Grab the char
					u_int16_t opt = left_opt->nfa_state_list.states[i]->opt;
					previous->transitions[opt] = left_opt;
				}
					
				//Patching in right_opt
				for(u_int16_t i = 0; i < right_opt->nfa_state_list.length; i++){
					//Grab the char
					u_int16_t opt = right_opt->nfa_state_list.states[i]->opt;
					previous->transitions[opt] = right_opt;
				}


				//We'll now need to navigate to the end of the right opt repeater
				//sub-DFA
				cursor = right_opt;

				
				while(cursor->next != NULL){
					cursor = cursor->next;
				}

				//Now that we're here, cursor holds the very end of the right sub-DFA
				//Everything that we have in the cursor must point back to the left DFA
				for(u_int16_t i = 0; i < left_opt->nfa_state_list.length; i++){
					//Grab the char
					u_int16_t opt = left_opt->nfa_state_list.states[i]->opt;
					cursor->transitions[opt] = left_opt;
				}
					
				//Patching in right_opt
				//Everything that we have in the cursor must also point back to the right DFA
				for(u_int16_t i = 0; i < right_opt->nfa_state_list.length; i++){
					//Grab the char
					u_int16_t opt = right_opt->nfa_state_list.states[i]->opt;
					cursor->transitions[opt] = right_opt;
				}
			

				/**	
				temp = previous;
				//Set this state to point back to itself
				previous->transitions[previous_opt] = previous;
				//Should already be reachable
				nfa_cursor->next->visited = 3;
				*/
				//Get out
				return dfa_start;

			default:
				temp = create_DFA_state(nfa_cursor);
				//Patch in all of our new states
				for(u_int16_t i = 0; i < temp->nfa_state_list.length; i++){
					//Grab the character
					u_int16_t opt = temp->nfa_state_list.states[i]->opt;
					//Patch in all of the new states
					previous->transitions[opt] = temp;
					printf("ADDED TRANSITION FOR: %d\n", opt);
				}

				//Advance the current DFA pointer
				previous->next = temp;
				previous = temp;

				//If we are flagging states, there may be times when we don't want to
				if(flag_states == 1){
					//We've now visited this state
					nfa_cursor->visited = 3;
				}

				//Advance the pointer
				nfa_cursor = nfa_cursor->next;
				break;
		}
	}

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
regex_t* define_regular_expression(char* pattern, regex_mode_t mode){
	//Stack allocate a regex
	regex_t* regex = malloc(sizeof(regex_t));
	//Set to NULL as a flag
	regex->NFA = NULL;
	regex->DFA = NULL;

	//Just in case
	if(pattern == NULL || strlen(pattern) == 0){
		//Verbose mode
		if(mode == REGEX_VERBOSE){
			printf("REGEX ERROR: Pattern cannot be null or empty\n");
		}

		//Set this flag so that the user knows
		regex->state = REGEX_ERR;
		return regex;
	}

	//Set a hard limit. I don't see a situation where we'd need more than 150 characters for a regex
	if(strlen(pattern) >= REGEX_LEN){
		//Verbose mode
		if(mode == REGEX_VERBOSE){
			printf("REGEX ERROR: Patterns of size 150 or more not supported\n");
		}

		regex->state = REGEX_ERR;
		return regex;
	}

	//Convert to postfix before applying our algorithm
	char* postfix = in_to_post(pattern, mode);
	//Save for reference
	regex->regex = postfix;

	//If this didn't work, we will stop and return a bad regex
	if(postfix == NULL){
		//Verbose mode
		if(mode == REGEX_VERBOSE){
			printf("REGEX ERROR: Postfix conversion failed.\n");
		}

		//Put in error state
		regex->state = REGEX_ERR;
		return regex;
	}

	//Show postfix if it exists
	if(mode == REGEX_VERBOSE){
		printf("Postfix conversion: %s\n", postfix);
	}

	//Initially 0
	regex->num_states = 0;

	//Create the NFA first
	regex->NFA = create_NFA(postfix, mode, &(regex->num_states));

	//If this is bad, we'll bail out here
	if(regex->NFA == NULL){
		if(mode == REGEX_VERBOSE){
			printf("REGEX ERROR: NFA creation failed.\n");
			//Put in an error state
			regex->state = REGEX_ERR;
			
			//Ensure there is no leakage
			free(postfix);

			return regex;
		}
	}

	//Display if desired
	if(mode == REGEX_VERBOSE){
		printf("\nNFA conversion succeeded.\n");
		print_NFA(regex->NFA);
		printf("\n\nBeginning DFA Conversion.\n\n");
	}

	//Now we'll use the NFA to create the DFA. We'll do this because DFA's are much more
	//efficient to simulate since they are determinsitic, but they are much harder to create
	//from regular expressions
	regex->DFA = create_DFA(regex->NFA, mode, 0);

	//If it didn't work
	if(regex->DFA == NULL){
		if(mode == REGEX_VERBOSE){
			printf("REGEX ERROR: DFA creation failed.\n");
			regex->state = REGEX_ERR;
			free(postfix);
			return regex;
		}
	}

	//Display if desired
	if(mode == REGEX_VERBOSE){
		printf("DFA conversion succeeded.\n");
		print_DFA(regex->DFA);
		printf("\n");
	}

	//If it did work, we'll set everything to true
	regex->state = REGEX_VALID;

	//If the user request verbose mode, we'll display
	if(mode == REGEX_VERBOSE){
		printf("regex_t creation succeeded. Regex is now ready to be used.\n");
	}

	//If we make it here, we did a valid postfix, NFA and then NFA->DFA conversion, so
	//the regex is all set
	return regex;
}


/**
 * A helper function that will simulate the running of the DFA to create matching //FIXME
 */
static void match(regex_match_t* match, regex_t* regex, char* string, u_int32_t starting_index, regex_mode_t mode){
	//Advance the string pointer to be at the starting index the user asked for
	char* match_string = string + starting_index;

	//By default, we haven't found anything
	match->status = MATCH_NOT_FOUND;
	//Initialize this to the starting index
	match->match_start_idx = starting_index;
	//By default, these match meaning we don't have a match
	match->match_end_idx = starting_index;

	//Store a reference to the current state
	DFA_state_t* start_state = (DFA_state_t*)(regex->DFA);
	//By defualt, we are in the starting state
	DFA_state_t* current_state = start_state;

	char ch;
	//The current index for the search
	u_int32_t current_index = starting_index;
	//Scan through the string
	while((ch = *match_string) != '\0'){

		//For each character, we'll attempt to advance using the transition list. If the transition list at that
		//character does not=NULL(0, remember it was calloc'd), then we can advance. If it is 0, we'll reset the search
		if(current_state->transitions[(u_int16_t)ch] != NULL){
			//If we're in verbose mode, print this out
			if(mode == REGEX_VERBOSE && current_state->transitions[(u_int16_t)ch] != NULL){
				printf("Pattern continued/started with character: %c\n", ch);
			}

			//Advance the current index
			current_index++;
			//We are in the start of a match, so we'll save the end state
			match->match_end_idx = current_index;

			//Advance this up to be the next state
			if(current_state->transitions[(u_int16_t)ch] != NULL){
				current_state = current_state->transitions[(u_int16_t)ch];

			} else {
				if(mode == REGEX_VERBOSE){
					//Reset these two parameters to reset the search
					match->match_start_idx = current_index;
					match->match_end_idx = current_index;

					//We are now back in the start state
					current_state = start_state;

					printf("No pattern found for character: %c\n", ch);
				}
			}
		
		} else if(current_state->transitions[ACCEPTING] != NULL){
			current_state = current_state->transitions[ACCEPTING];
		
		//Otherwise, we didn't find anything, so we need to reset
		} else {
			//Print out if we're in verbose mode
			if(mode == REGEX_VERBOSE){
				printf("No pattern found for character: %c\n", ch);
			}

			//Advance the current index
			current_index++;

			//Reset these two parameters to reset the search
			match->match_start_idx = current_index;
			match->match_end_idx = current_index;

			//We are now back in the start state
			current_state = start_state;
		}

		if(current_state == NULL) printf("I AM NULL");
		if(current_state->nfa_state_list.contains_accepting_state == 1){
			//We've found the match
			match->status = MATCH_FOUND;

			if(mode == REGEX_VERBOSE){
				printf("Match found!\n");
				return;
			}
		}

		//Push the pointer up
		match_string++;
	}

	//Case that we have an "end match"
	if(current_state->transitions[ACCEPTING] != NULL){
		current_state = current_state->transitions[ACCEPTING];
	}

	if(current_state->nfa_state_list.contains_accepting_state == 1){
		//We've found the match
		match->status = MATCH_FOUND;

		if(mode == REGEX_VERBOSE){
			printf("Match found!\n");
			return;
		}
	}



	//If we end up here, that means that we ran off the end of the string and never found a match
	//We'll set these flags and return if this is the case
	match->match_end_idx = starting_index;
	match->match_end_idx = starting_index;
	match->status = MATCH_NOT_FOUND;
	return;
}


/**
 * The public facing match method that the user will call when attempting to pattern match
 */
regex_match_t regex_match(regex_t* regex, char* string, u_int32_t starting_index, regex_mode_t mode){
	//Stack allocated match struct
	regex_match_t match_struct;

	//Error mode by default
	match_struct.status = MATCH_ERR;

	//If we are given a bad regex 
	if(regex->DFA == NULL || regex->state == REGEX_ERR){
		//Verbose mode
		if(mode == REGEX_VERBOSE){
			printf("REGEX ERROR: Attempt to use an invalid regex.\n");
		}

		//Pack in the values and return match.match_start = 0;
		match_struct.match_start_idx = 0;
		match_struct.match_end_idx = 0;

		//We return this value so that the caller can know what the error was
		match_struct.status = MATCH_INV_INPUT;

		//Give the regex back
		return match_struct;
	}

	//If we are given a bad string
	if(string == NULL || strlen(string) == 0){
		//Verbose mode
		if(mode == REGEX_VERBOSE){
			printf("REGEX ERROR: Attempt to match a NULL string or a string of length 0.\n");
		}

		//Pack in the values and return
		match_struct.match_start_idx = 0;
		match_struct.match_end_idx = 0;
		match_struct.status = MATCH_INV_INPUT;
		return match_struct;
	}

	//Attempt to match the string with the regex
	match(&match_struct, regex, string, starting_index, mode);


	//Return the match struct
	return match_struct;
}


/* ================================================== Cleanup ================================================ */


/**
 * Iteratively free all NFA states using their built in linked list feature.
 * The linked list feature allows for us to avoid spirious loops that can
 * occur because states are allowed to point back to themselves. All that we
 * have to do with this is follow the chain down to the end
 */
static void teardown_NFA(NFA_state_t* state_ptr){
	//Base case
	if(state_ptr == NULL){
		return;
	}

	//Two pointers for our use here
	NFA_state_t* cursor = state_ptr;
	NFA_state_t* temp;

	//Loop while we still have stuff to free
	while(cursor != NULL) {
		//Save the value
		temp = cursor;
		//Advance the linked list
		cursor = cursor->next_created;
		//Free the value
		free(temp);
	}
}


/**
 * Iteratively free all DFA states that are pointed to. We should have no dangling states, so in theory,
 * this should work
 */
static void teardown_DFA(DFA_state_t* state){
	//A cursor for us to use
	DFA_state_t* cursor = state;
	DFA_state_t* temp;

	while(cursor != NULL){
		//Save cursor
		temp = cursor;
		//Advance the pointer
		cursor = cursor->next;

		//Free temp after we've advanced
		free(temp);
	}
}


/**
 * Comprehensive cleanup function that cleans up everything related to the regex
 */
void destroy_regex(regex_t* regex){
	//Teardown the NFA
	teardown_NFA((NFA_state_t*)(regex->NFA));

	//Clean up the DFA
	teardown_DFA((DFA_state_t*)(regex->DFA));

	//Free the postfix expression
	free(regex->regex);

	//Free the regex itself
	free(regex);
}


/* ================================================== Cleanup ================================================ */
