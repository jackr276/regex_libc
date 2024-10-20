/**
 * Author: Jack Robbins
 * This file contains the implementation for the regex library API defined in 
 * regex.h . Specifically, this file with procedurally generate a state machine that recognizes
 * strings belonging to a regular expression
 */

#include "regex.h" 
#include <stdio.h>
#include <string.h>

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
	u_int8_t visited;
	//The char that we hold
	u_int16_t opt;
	//The default next 
	NFA_state_t* next;
	//The optional second next for alternating states 
	NFA_state_t* next_opt;
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
	NFA_state_t* states[130];
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
	DFA_state_t* transitions[130];
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
 * Path the list of states contained in the arrow_list out to point to the start state
 * of the next fragement "start"
 *
 * point_opt will make use of next if 1, next_opt if 0
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
	if(nfa == NULL){
		return;
	}

	//Support printing of special characters split and accepting
	if(nfa->opt == SPLIT){
		printf("State -SPLIT->");
	} else if(nfa->opt == ACCEPTING){
		printf("State -ACCEPTING->");
	} else {
		printf("State -%c->", (u_int8_t)nfa->opt);
	}

	if(nfa->opt == SPLIT){
		print_NFA(nfa->next);
		print_NFA(nfa->next_opt);
	} else {
		print_NFA(nfa->next);
	}
}


/**
 * Ability to print out a DFA for debug purposes
 */
static void print_DFA(DFA_state_t* dfa){
	//If the state is null we'll stop
	if(dfa == NULL){
		return;
	}

	printf("States: {");

	for(u_int16_t i = 0; i < dfa->nfa_state_list.length; i++){
		(dfa->nfa_state_list.states[i]->opt == ACCEPTING) ? printf("ACCEPTING") : printf("%c, ", dfa->nfa_state_list.states[i]->opt);
	}

	printf("}->");

	for(u_int16_t i = 0; i < 130; i++){
		if(dfa->transitions[i] != NULL){
			print_DFA(dfa->transitions[i]);
		}
	}
}


/**
 * Create an NFA from a postfix regular expression FIXME does not work for () combined with *, | or +
 */
static NFA_state_t* create_NFA(char* postfix, regex_mode_t mode, u_int16_t* num_states){
	//Create a stack for pushing/popping
	stack_t* stack = create_stack();

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
				num_processed++;
				//Grab the two most recent fragments off of the stack
				frag_2 = (NFA_fragement_t*)pop(stack);
				frag_1 = (NFA_fragement_t*)pop(stack);

				//Create a new special "split" state that acts as a fork in the road between the two
				//fragment start states
				split = create_state(SPLIT, frag_1->start,  frag_2->start, num_states);

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
				num_processed++;

				//Pop the most recent fragment
				frag_1 = pop(stack);

				//Create a new state. This new state will act as our split. This state will point to the start of the fragment we just got
				split = create_state(SPLIT, frag_1->start, NULL, num_states);

				//Print out the fringe states DEBUGGING STATEMENT
				if(mode == REGEX_VERBOSE){
					print_fringe_states(frag_1->fringe_states);
				}

				//Make all of the states in fragment_1 point to the beginning of the split 
				//using their next_opt to allow for our "0 or more" functionality 
				concatenate_states(frag_1->fringe_states, split, 1);

				//Create a new fragment that originates at the new state, allowing for our "0 or many" function here
				push(stack, create_fragment(split, concatenate_lists(frag_1->fringe_states, init_list(split->next_opt))));

				//Free this pointer as it is no longer needed
				free(frag_1);

				break;

			//1 or more, more specifically positive closure
			case '+':
				//We should try handing this instead as "char`char+" because that's really what it is
				num_processed++;

				//Grab the most recent fragment
				frag_1 = pop(stack);

				//We'll create a new state that acts as a split, going back to the the original state
				//This acts as our optional 1 or more 
				split = create_state(SPLIT, frag_1->start, NULL, num_states);

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
				push(stack, create_fragment(frag_1->start, concatenate_lists(frag_1->fringe_states, init_list(split->next_opt))));

				//Free this pointer
				free(frag_1);

				break;

			//0 or 1 instances of the preceding fragment
			case '?':
				num_processed++;

				//Grab the most recent fragment
				frag_1 = pop(stack);

				//We'll create a new state that acts as a split, but this time we won't add any arrows back to this
				//state. This allows for a "zero or one" function
				split = create_state(SPLIT, frag_1->start, NULL, num_states);

				//Note how for this one, we won't concatenate states at all, but we'll instead concatentate
				//the two fringe lists into one big one because the fringe is a combined fringe
				fringe = concatenate_lists(frag_1->fringe_states, init_list(split->next_opt));
				
				//Create a new fragment that starts at the split, and represents this whole structure. We also need to chain the lists together to keep everything connected
				push(stack, create_fragment(split, fringe));

				//We won't free the fragments list here because it still holds fringe data
				//Free this pointer
				free(frag_1);

				break;

			//If we see the escape character, then we process the immediately next character as a regular char
			case '~':
				//One more processed
				num_processed++;

				//We'll skip over this character, the one that's next is what really matters
				cursor++;

				//Create a new state with the escaped character
				s = create_state(*cursor, NULL,  NULL,  num_states);

				//Create a fragment with the fringe states being the new state that we created
				fragment = create_fragment(s, init_list(s));

				//Push this new fragment to the stack
				push(stack, fragment);

				break;

			//Any character that is not one of the special characters
			default:
				//One more processed
				num_processed++;

				//Create a new state with the charcter, and no attached states
				s = create_state(ch, NULL, NULL, num_states);

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


/* ================================================== DFA Methods ================================================ */


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

	//If we find an accepting state, then set this flag. This will speed up our match function
	if(start->opt == ACCEPTING){
		list->contains_accepting_state = 1;
	}

	//Increment the length
	list->length++;
}


/**
 * Initialize a DFA state list by reading all of the transitions of states reachable directly from "start". This new
 * list will be stored in "list". "num_states" is the number of DFA states, which should in theory be the maximum number of 
 * states we could possibly have in one of our DFA state lists
 */
static void get_all_reachable_states(NFA_state_t* start, NFA_state_list_t* state_list, u_int16_t num_states){
	//Currently there's nothing, so we'll set this to 0
	state_list->length = 0;

	//By default, we don't have an accepting state in our list
	state_list->contains_accepting_state = 0;

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

	//0 out the entire array of DFA transitions as well
	memset(dfa_state->transitions, 0, 130*sizeof(DFA_state_t*));

	//0 out the entire array of NFA states
	memset(dfa_state->nfa_state_list.states, 0, 130*sizeof(NFA_state_t*));

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
	if(nfa_state == NULL || nfa_state->visited == 1){
		//"dead end" so to speak
		return;
	}

	//We have visited this guy already now
	nfa_state->visited = 1;

	//TODO make him regex_verbose only
	printf("Creating state for opt: %c\n", nfa_state->opt);

	//Create the new DFA state
	DFA_state_t* new_state = create_DFA_state(nfa_state, num_states);

	//Iterate over the entire NFA state list to "patch in" everything that we need here
	//WORKS
	for(u_int16_t i = 0; i < new_state->nfa_state_list.length; i++){
		//We want everything in the previous state to point to the new state
		if(new_state->nfa_state_list.states[i] != NULL){
			//Grab the option
			u_int16_t opt = new_state->nfa_state_list.states[i]->opt;

			//We don't want any non-characters polluting the array
			if(opt != 0){
				previous->transitions[opt] = new_state;
			}
		}
	}

	//Recursively create the next DFA state for opt and next opt
	if(nfa_state->opt != SPLIT){
		//We should only create these if we don't have a split
		create_DFA_rec(new_state, nfa_state->next, num_states);
		create_DFA_rec(new_state, nfa_state->next_opt, num_states);
	} else {
		//If we get here, we'll skip over the states that were already picked up by the split and go onto 
		//the next
		if(nfa_state->next != NULL){
			create_DFA_rec(previous, nfa_state->next->next, num_states);
			create_DFA_rec(previous, nfa_state->next->next_opt, num_states);
		}

		//Recursively create the states from next_opt
		if(nfa_state->next_opt != NULL){
			create_DFA_rec(previous, nfa_state->next_opt->next, num_states);
			create_DFA_rec(previous, nfa_state->next_opt->next_opt, num_states);
		}
	}
}


/**
 * Translate an NFA into an equivalent DFA
 */
static DFA_state_t* create_DFA(NFA_state_t* nfa_start, u_int16_t num_states){
	//We'll explicitly create the start state here
	DFA_state_t* dfa_start = (DFA_state_t*)malloc(sizeof(DFA_state_t));

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
	regex.DFA = NULL;

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

	//Display if desired
	if(mode == REGEX_VERBOSE){
		printf("\nNFA conversion succeeded.\n");
		print_NFA(regex.NFA);
		printf("\n\nBeginning DFA Conversion.\n\n");
	}

	//Now we'll use the NFA to create the DFA. We'll do this because DFA's are much more
	//efficient to simulate since they are determinsitic, but they are much harder to create
	//from regular expressions
	regex.DFA = create_DFA(regex.NFA, mode);

	//If it didn't work
	if(regex.DFA == NULL){
		if(mode == REGEX_VERBOSE){
			printf("REGEX ERROR: DFA creation failed.\n");
			regex.state = REGEX_ERR;
			free(postfix);
			return regex;
		}
	}

	//Display if desired
	if(mode == REGEX_VERBOSE){
		printf("DFA conversion succeeded.\n");
		print_DFA(regex.DFA);
		printf("\n");
	}

	//If it did work, we'll set everything to true
	regex.state = REGEX_VALID;

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
			if(mode == REGEX_VERBOSE){
				printf("Pattern continued/started with character: %c\n", ch);
			}

			//Advance the current index
			current_index++;
			//We are in the start of a match, so we'll save the end state
			match->match_end_idx = current_index;

			//Advance this up to be the next state
			current_state = current_state->transitions[(u_int16_t)ch];
		
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

		//If the current state contains the accepting state, this is our base case
		if(current_state->transitions[ACCEPTING] != 0){
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


	//If we end up here, that means that we ran off the end of the string and never found a match
	//We'll set these flags and return if this is the case
	match->match_end_idx = starting_index;
	match->match_end_idx = starting_index;
	match->status = MATCH_NOT_FOUND;
	return;
}


/**
 * The public facing match method that the user will call when attempting to pattern match
 * 
 */
regex_match_t regex_match(regex_t regex, char* string, u_int32_t starting_index, regex_mode_t mode){
	//Stack allocated match struct
	regex_match_t match_struct;

	//Error mode by default
	match_struct.status = MATCH_ERR;

	//If we are given a bad regex 
	if(regex.DFA == NULL || regex.state == REGEX_ERR){
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
	match(&match_struct, &regex, string, starting_index, mode);


	//Return the match struct
	return match_struct;
}


/* ================================================== Cleanup ================================================ */


/**
 * Recursively free all NFA states in that are pointed to. We should have no dangling states, so 
 * in theory, this should work. We'll keep track of what the accepting state is because there is a chance
 * that we could double free it, so we won't let the recursive function handle that
 */
static void teardown_NFA_state(NFA_state_t** state_ptr, NFA_state_t** accepting_state){
	//Base case
	if(state_ptr == NULL || *state_ptr == NULL){ 
		return;
	}

	//If we find the accepting state we will save it for later to be freed by
	//the caller
	if((*state_ptr)->opt == ACCEPTING){
		*accepting_state = *state_ptr;
		return;
	}

	//Recursively call free on the next states here
	if((*state_ptr)->next != NULL){
		teardown_NFA_state(&((*state_ptr)->next), accepting_state);
	}

	if((*state_ptr)->next_opt != NULL){
		teardown_NFA_state(&((*state_ptr)->next_opt), accepting_state);
	}

	//Free the pointer to this state
	free(*state_ptr);
	//Set to NULL as a warning
	*state_ptr = NULL;
}


/**
 * Recursively free all DFA states that are pointed to. We should have no dangling states, so in theory,
 * this should work
 */
static void teardown_DFA_state(DFA_state_t** state){
	//Base case
	if(*state == NULL){
		return;
	}

	//Recursively teardown every other state
	for(u_int16_t i = 0; i < 130; i++){
		teardown_DFA_state(&((*state)->transitions[i]));
	}

	//Free the state overall
	free(*state);

	*state = NULL;
}


/**
 * Comprehensive cleanup function that cleans up everything related to the regex
 */
void destroy_regex(regex_t regex){
	//Call the recursive NFA freeing function
	NFA_state_t* accepting_state;

	teardown_NFA_state((NFA_state_t**)(&(regex.NFA)), &accepting_state);
	//Once this function runs, we should have a reference to the accepting state that we can free

	//Prevent any double frees of the accepting state
	if(accepting_state != NULL){
		free(accepting_state);
	}
	
	//Clean up the DFA
	teardown_DFA_state((DFA_state_t**)(&(regex.DFA)));
}


/* ================================================== Cleanup ================================================ */
