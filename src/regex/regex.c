/**
 * Author: Jack Robbins
 * This file contains the implementation for the regex library API defined in 
 * regex.h . Specifically, this file will generate a state machine that recognizes
 * strings belonging to a regular expression
 */ #include "regex.h" 
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
	NFA_state_t* states[145];
	u_int16_t length;
	//Does this list contain an accepting state?
	u_int8_t contains_accepting_state;
	//Does this list have a wildcard?
	u_int8_t contains_wild_card;
	//Does this list have a NUMBERS state?
	u_int8_t contains_numbers;
	//Does this list containt LOWERCASE?
	u_int8_t contains_lowercase;
	//Does this state contain uppercase?
	u_int8_t contains_uppercase;
	//Does this have all the letters
	u_int8_t contains_letters;
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
	DFA_state_t* transitions[145];
	//The next dfa_state that was made, this will help us in freeing
	DFA_state_t* next;
};

/**
 * An improved version of the postfix converter using an operator stack
 */
char* in_to_post(char* regex, regex_mode_t mode){
	//Sanity check
	if(regex == NULL || strlen(regex) == 0){
		if(mode == REGEX_VERBOSE){
			printf("ERROR: Null regex passed in\n");
		}
		//Get out 
		return NULL;
	}

	//Check that all characters passed in are printable characters. If they aren't
	//simply exit
	for(char* regex_cursor = regex; *regex_cursor != '\0'; regex_cursor++){
		if(*regex_cursor < 32 || *regex_cursor > 126){
			if(mode == REGEX_VERBOSE){
				printf("ERROR: Non-printable character passed in\n");
			}
			return NULL;
		}
	}

	//Now that we know that we are in the clear here, we can begin allocating some stuff
	//Allocate plenty of space for ourselves here
	char* regex_with_concatenation = calloc(strlen(regex) * 5, sizeof(char));
	/**
	 * We will now go through and add in the explicit concatenation characters(`)
	 * The rules for adding these are as follows:
	 * 	1.) Is the next character a regular char or not?
	 */

	//Preserve the original pointer
	char* cursor = regex;
	char* concat_cursor = regex_with_concatenation;

	//Keep track of the last thing that we saw
	char previous_char = '\0';
	
	//Go through and add everything into the regex_with_concatenation string
	while(*cursor != '\0'){
		switch(*cursor){
			//For all of these, we'll just attach it and move on
			case '*':
			case '+':
			case '?':
			case '|':
			case ')':
				//Add it in and move along
				previous_char = *cursor;
				*concat_cursor = *cursor;
				cursor++;
				concat_cursor++;

				break;
			//Handle an open parenthesis
			case '(':
				//This is the one case that we will not concatenate
				if(previous_char == '\0' || previous_char == '|' || previous_char == '('){
					*concat_cursor = *cursor;
					cursor++;
					concat_cursor++;
					break;
				}

				//We'll have to concatenate here
				*concat_cursor = '`';
				concat_cursor++;

				//Add in the parenthesis
				previous_char = *cursor;
				*concat_cursor = *cursor;
				cursor++;
				concat_cursor++;

				break;
			//This is our escape character
			case '\\':
				//We can add in concatenation here
				if(previous_char != '\0' && previous_char != '(' && previous_char != '|'){
					*concat_cursor = '`';
					concat_cursor++;
				}

				*concat_cursor = '\\';
				//We can use this as the previous char
				previous_char = '\\';
				concat_cursor++;
				cursor++;
				//Add whatever we had in
				*concat_cursor = *cursor;
				concat_cursor++;
				cursor++;

				break;
			//We can see [0-9] or [a-z] or [A-Z] or [a-zA-z]
			case '[':
				//If the previous char was an open paren, we won't
				//add a concatenation
				if(previous_char !='\0' && previous_char != '(' && previous_char != '|'){
					*concat_cursor = '`';
					concat_cursor++;
				}

				cursor++;

				if(*cursor == '0'){
					//We need to see this sequence, otherwise it's bad
					if(*(cursor + 1) != '-' || *(cursor + 2) != '9' || *(cursor+3) != ']'){
						if(mode == REGEX_VERBOSE){
							printf("ERROR: Invalid range provided\n");
						}
						//This is bad so we'll get out
						return NULL;
					}

				} else if (*cursor == 'a'){
					//We need to see this sequence, otherwise it's bad
					if(*(cursor + 1) != '-' || *(cursor + 2) != 'z'){
						if(mode == REGEX_VERBOSE){
							printf("ERROR: Invalid range provided\n");
						}
						//This is bad so we'll get out
						return NULL;
					}
					//Let's see which we actually have here
					if(*(cursor+3) == ']'){
						//Jump to our basic range
						goto basic_range;
					} else {
						//Otherwise we should have A-Z left
						if(*(cursor + 3) != 'A' || *(cursor + 4) != '-' || *(cursor + 5) != 'Z'){
							if(mode == REGEX_VERBOSE){
								printf("ERROR: Invalid range provided\n");
							}
							//This is bad so we'll get out
							return NULL;
						}

						//Add this on
						strcat(regex_with_concatenation, "[a-zA-Z]");
						previous_char = ']';
						
						//Jump ahead here
						concat_cursor += 8;
						//Jump ahead
						cursor += 7;
						
						break;
					}

				} else if (*cursor == 'A'){
					//We need to see this sequence, otherwise it's bad
					if(*(cursor + 1) != '-' || *(cursor + 2) != 'Z' || *(cursor+3) != ']'){
						if(mode == REGEX_VERBOSE){
							printf("ERROR: Invalid range provided\n");
						}
						//This is bad so we'll get out
						return NULL;
					}

				} else {
					if(mode == REGEX_VERBOSE){
						printf("ERROR: Invalid range provided\n");
					}

					//This is bad so we'll get out
					return NULL;
				}

			basic_range:
				//Go through and add them in
				*concat_cursor = '[';
				concat_cursor++;
				//These can vary based on what is actually in here
				*concat_cursor = *cursor;
				concat_cursor++;
				*concat_cursor = *(cursor + 1);
				concat_cursor++;
				*concat_cursor = *(cursor + 2);
				concat_cursor++;
				*concat_cursor = ']';
				concat_cursor++;

				//Record the previous char here
				previous_char = ']';

				//Move ahead
				cursor += 4;

				break;
					
			//Now we can handle all of our letters
			default:
				//If the previous char was an open paren, we won't
				//add a concatenation
				if(previous_char != '\0' && previous_char != '(' && previous_char != '|'){
					*concat_cursor = '`';
					concat_cursor++;
				}

				//Save the previous char
				previous_char = *cursor;

				//Add back in
				*concat_cursor = *cursor;
				concat_cursor++;
				cursor++;
				break;
		}
	}

	//Display if we're in verbose mode
	if(mode == REGEX_VERBOSE){
		printf("With concatenation characters added: %s\n", regex_with_concatenation);
	}
	
	/**
	 * Now that we've added in explicit concatenation, we can go through and use
	 * the Shunting-Yard algorithm to create the postfix expression
	 */

	//This will eventually be used for our postfix display
	char* postfix = calloc(strlen(regex)*5, sizeof(char));
	//Restart the concat cursor
	concat_cursor = regex_with_concatenation;
	//This ensures we don't lose the start
	char* postfix_cursor = postfix;
	char stack_cursor;
	u_int8_t found_open;

	//An operator stack that will hold any operators that we see
	stack_t* operator_stack = create_stack();

	/**
	 * Shunting Yard Algorithm:
	 * 	Go through the infix expression char by char
	 * 	If we see an operator, compare that operator with the operator stack
	 * 		If the operator on the top of the stack has a higher precedence, pop it off
	 * 		and append to the string. If not, push the new operator onto the stack.
	 *
	 *  At the very end, we pop all operators off of the stack and place them at the end of the 
	 *  string
	 */
	//Go through the regex with concatenation string
	while(*concat_cursor != '\0'){
		//Switch based on what character we see
		switch(*concat_cursor){
			//If we see the escape character, it and whatever
			//come after are treated just like regular characters
			case '\\':
				//Add the explicit escape char in
				*postfix_cursor = *concat_cursor;
				postfix_cursor++;
				concat_cursor++;
				//Add whatever was escaped in too
				*postfix_cursor = * concat_cursor;
				postfix_cursor++;
				concat_cursor++;

				break;

			//Handle kleene start operator
			case '*':
				//As long as we haven't hit the bottom
				while(peek(operator_stack) != NULL){
					//Pop off of the stack
					stack_cursor = *((char*)peek(operator_stack));
					//If it's of equal or higher precedence(?, * or +) we'll pop it off
					if(stack_cursor == '*' || stack_cursor == '+' || stack_cursor == '?'){
						//Pop it and add it on to the postfix
						*postfix_cursor = *((char*)pop(operator_stack));
						postfix_cursor++;
					//Otherwise it's of lower precedence so we'll leave
					} else {
						break;
					}
				}

				//This will always get pushed on
				push(operator_stack, "*");
				
				//Advance the cursor
				concat_cursor++;
				break;

			//Handle positive closure operator
			case '+':
				//As long as we haven't hit the bottom
				while(peek(operator_stack) != NULL){
					//Pop off of the stack
					stack_cursor = *((char*)peek(operator_stack));
					//If it's of equal or higher precedence(?, * or +) we'll pop it off
					if(stack_cursor == '*' || stack_cursor == '+' || stack_cursor == '?'){
						//Pop it and add it on to the postfix
						*postfix_cursor = *((char*)pop(operator_stack));
						postfix_cursor++;
					//Otherwise it's of lower precedence so we'll leave
					} else {
						break;
					}
				}

				//This will always get pushed on
				push(operator_stack, "+");
				//Advance the cursor
				concat_cursor++;
				
				break;

			//Handle 0 or 1 operator
			case '?':
				//As long as we haven't hit the bottom
				while(peek(operator_stack) != NULL){
					//Pop off of the stack
					stack_cursor = *((char*)peek(operator_stack));
					//If it's of equal or higher precedence(?, * or +) we'll pop it off
					if(stack_cursor == '*' || stack_cursor == '+' || stack_cursor == '?'){
						//Pop it and add it on to the postfix
						*postfix_cursor = *((char*)pop(operator_stack));
						postfix_cursor++;
					//Otherwise it's of lower precedence so we'll leave
					} else {
						break;
					}
				}

				//This will always get pushed on
				push(operator_stack, "?");
				
				//Advance the cursor
				concat_cursor++;
				break;

			case '`':
				//As long as we haven't hit the bottom
				while(peek(operator_stack) != NULL){
					//Pop off of the stack
					stack_cursor = *((char*)peek(operator_stack));
					//If it's of equal or higher precedence(?, * or +) we'll pop it off
					if(stack_cursor == '*' || stack_cursor == '+' || stack_cursor == '?'
					   || stack_cursor == '`'){
						//Pop it and add it on to the postfix
						*postfix_cursor = *((char*)pop(operator_stack));
						postfix_cursor++;
					//Otherwise it's of lower precedence so we'll leave
					} else {
						break;
					}
				}

				//This will always get pushed on
				push(operator_stack, "`");
				//Advance the cursor
				concat_cursor++;
				
				break;
			
			//Handle the union(|) symbol	
			case '|':
				//As long as we haven't hit the bottom
				while(peek(operator_stack) != NULL){
					//Pop off of the stack
					stack_cursor = *((char*)peek(operator_stack));
					//If it's of equal or higher precedence(?, * or + or '`' or `|`) we'll pop it off
					if(stack_cursor == '*' || stack_cursor == '+' || stack_cursor == '?'
					   || stack_cursor == '`' || stack_cursor == '|'){
						//Pop it and add it on to the postfix
						*postfix_cursor = *((char*)pop(operator_stack));
						postfix_cursor++;
					//Otherwise it's of lower precedence so we'll leave
					} else {
						break;
					}
				}

				//This will always get pushed on
				push(operator_stack, "|");
				//Advance the cursor
				concat_cursor++;

				break;
			
			//Handle left parenthesis
			case '(':
				//If we see this, we simply push it onto the stack and keep moving
				push(operator_stack, "(");
				//Move on to next character
				concat_cursor++;

				break;

			//Handle the closing parenthesis
			case ')':
				//Once we see these, we'll need to keep popping off of the stack
				//until we either run out or we hit a closing parenthesis
				found_open = 0;

				//As long as we haven't hit the bottom
				while(peek(operator_stack) != NULL){
					//Pop off of the stack
					stack_cursor = *((char*)pop(operator_stack));

					//If it's an end paren then we're done
					if(stack_cursor == '('){
						found_open = 1;
						break;
					} else {
						//Otherwise, append the operator that we have to the postfix string
						*postfix_cursor = stack_cursor;
						postfix_cursor++;
					}
				}

				//If this happens, we had an unmatched closing parenthesis. We'll cleanup and get out
				if(found_open == 0){
					printf("ERROR: Unmatched closing parenthesis");
					//Free this because we know it's bad
					free(postfix);
					//Cleanup
					destroy_stack(operator_stack, STATES_ONLY);
					return NULL;
				}
				
				//Advance the cursor
				concat_cursor++;
				break;

			//If we're here, we have a regular character so we just append it
			default:
				*postfix_cursor = *concat_cursor;
				concat_cursor++;
				postfix_cursor++;
				break;
		}
	}

	//Once we reach the end, we'll need to pop everything else that we have on the stack off and append it
	//As long as we haven't hit the bottom
	while(peek(operator_stack) != NULL){
		//Pop off of the stack
		stack_cursor = *((char*)pop(operator_stack));

		//If we get this, it means that we have an unmatched parenthesis
		if(stack_cursor == '('){
			printf("ERROR: Unmatched opening parenthesis");
			//Free this because we know it's bad
			free(postfix);
			//Cleanup
			destroy_stack(operator_stack, STATES_ONLY);
			return NULL;
		}

		//Otherwise, append the operator that we have to the postfix string
		*postfix_cursor = stack_cursor;
		postfix_cursor++;
	}

	//Destroy the whole stack
	destroy_stack(operator_stack, STATES_ONLY);
	
	//We don't need this anymore
	free(regex_with_concatenation);

	//Display if the user wants
	if(mode == REGEX_VERBOSE){
		printf("Postfix regular expression: %s\n", postfix);
	}

	return postfix;
}


/* ================================================== NFA Methods ================================================ */


/**
 * Create and return a state
 */
static NFA_state_t* create_state(u_int32_t opt, NFA_state_t* next, NFA_state_t* next_opt){
	//Allocate a state
 	NFA_state_t* state = (NFA_state_t*)calloc(1, sizeof(NFA_state_t));

	//Assign these values
	state->visited = 0;
	state->opt = opt;
	state->next = next;
	state->next_opt = next_opt;
	//Must be set later on
	state->next_created = NULL;

	//Give the pointer back
	return state;
}


/**
 * Create and return a fragment. A fragment is a partially built NFA. Our system works by building
 * consecutive fragments on top of previous fragments
 */
static NFA_fragement_t* create_fragment(NFA_state_t* start, fringe_states_t* fringe_states){
	//Allocate our fragment
	NFA_fragement_t* fragment = (NFA_fragement_t*)calloc(1, sizeof(NFA_fragement_t));

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
	fringe_states_t* list = calloc(1, sizeof(fringe_states_t));

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
 * Create a new state in memory that is completely identical to "state"
 */
static NFA_state_t* copy_state(NFA_state_t* state){
	//Create a new state
	NFA_state_t* copy = (NFA_state_t*)calloc(1, sizeof(NFA_state_t));

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
static NFA_fragement_t* copy_fragment(NFA_fragement_t* frag, NFA_state_t** tail){
	//Create the fragment copy
	NFA_fragement_t* copy = (NFA_fragement_t*)calloc(1, sizeof(NFA_fragement_t));
	NFA_state_t* copied_state = copy_state(frag->start);
	
	//Attach this to the linked list
	(*tail)->next_created = copied_state;
	(*tail) = copied_state;
	
	//Copy the start state
	copy->start = copied_state;

	return copy;
}

/**
 * Create an NFA from a postfix regular expression FIXME does not work for () combined with *, | or +
 */
static NFA_state_t* create_NFA(char* postfix, regex_mode_t mode){
	//Create a stack for pushing/popping
	stack_t* stack = create_stack();
	//A linked list for us to hold all of our created states
	//The very end of our linked list
	NFA_state_t* tail =  NULL;
	NFA_state_t* head = NULL;

	//Declare these for use 
	NFA_fragement_t* frag_2;
	NFA_fragement_t* frag_1;
	NFA_fragement_t* fragment;
	NFA_state_t* split;
	NFA_state_t* s;

	//Declare this for our use as well
	fringe_states_t* fringe;

	//Iterate until we hit the null terminator
	for(char* cursor = postfix; *cursor != '\0'; cursor++){
		//Grab the current char
		char ch = *cursor;

		//Switch on the character
		switch(ch){
			//Concatenation character
			case '`':
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

				//We're done with these now, so we should free them
				free(frag_1);
				free(frag_2);

				break;

			//Alternate state
			case '|':
				//Grab the two most recent fragments off of the stack
				frag_2 = (NFA_fragement_t*)pop(stack);
				frag_1 = (NFA_fragement_t*)pop(stack);

				//Create a new special "split" state that acts as a fork in the road between the two
				//fragment start states
				split = create_state(SPLIT_ALTERNATE, frag_1->start,  frag_2->start);

				//Special case due to the way in which split states work
				NFA_state_t* temp = head;
				head = split;
				head->next_created = temp;

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
				split = create_state(SPLIT_KLEENE, NULL, frag_1->start);
				
				//Store these in memory
				if(tail == NULL){
					head = tail = split;
				} else {
					tail->next_created = split;
					tail = split;
				}

				//Make all of the states in fragment_1 point to the beginning of the split 
				//using their next_opt to allow for our "0 or more" functionality 
				concatenate_states(frag_1->fringe_states, split, 1);

				//After concatenation, these are useless to us
				destroy_fringe_list(frag_1->fringe_states);

				//Create a new fragment that originates at the new state, allowing for our "0 or many" function here
				push(stack, create_fragment(split, init_list(split)));

				//Free this pointer as it is no longer needed
				free(frag_1);

				break;

			//1 or more, more specifically positive closure
			case '+':
				//Grab the most recent fragment
				frag_1 = pop(stack);
				frag_2 = copy_fragment(frag_1, &tail);

				//We'll create a new state that acts as a split, going back to the the original state
				//This acts as our optional 1 or more 
				split = create_state(SPLIT_POSITIVE_CLOSURE, NULL, frag_2->start);
	
				//If this is the very first state then it is our origin for the linked list
				if(tail == NULL){
					head = tail = split;
				} else {
					tail->next_created = split;
					tail = split;
				}

				//Set all of the fringe states in frag_1 to point at the split
				concatenate_states(frag_1->fringe_states, split, 1);
				
				//After concatenation, these are useless to us
				destroy_fringe_list(frag_1->fringe_states);

				//Create a new fragment that represent this whole structure and push to the stack
				//Since this one is "1 or more", we will have the start of our next fragment be the start of the old fragment
				//THIS is the problem here, we can't have this guy point to fragment->start. It has to point to the immediately preceeding
				//state
				push(stack, create_fragment(frag_1->start, init_list(split)));

				//Free this pointer
				free(frag_1);
				free(frag_2);

				break;

			//0 or 1 instances of the preceding fragment
			case '?':
				//Grab the most recent fragment
				frag_1 = pop(stack);

				//We'll create a new state that acts as a split, but this time we won't add any arrows back to this
				//state. This allows for a "zero or one" function
				//NOTE: Here, we'll use Split's next-opt to point back to the fragment at the start
				split = create_state(SPLIT_ZERO_OR_ONE, NULL, frag_1->start);

				//If this is the very first state then it is our origin for the linked list
				if(tail == NULL){
					head = tail = split;
				//We'll need to insert this into the linked list in the right position
				} else {
					//Add onto linked list
					tail->next_created = split;
					tail = split;
				}

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
			case '\\':
				cursor++;

				//Create a new state with the escaped character
				s = create_state(*cursor, NULL,  NULL);

				//Add to linked list
				if(tail == NULL){
					head = tail = s;
				} else {
					tail->next_created = s;
					tail = s;
				}

				//Create a fragment with the fringe states being the new state that we created
				fragment = create_fragment(s, init_list(s));

				//Push this new fragment to the stack
				push(stack, fragment);

				break;

			//Wildcard
			case '$':
				s = create_state(WILDCARD, NULL, NULL);
				//Concatenate to linked list
				if(tail == NULL){
					head = tail = s;
				} else {
					tail->next_created = s;
					tail = s;
				}

				//Create a fragment, with the fringe states of that fragment being just this new state that we
				//created
				fragment = create_fragment(s,  init_list(s));

				//Push the fragment onto the stack. We will pop it off when we reach operators
				push(stack, fragment);
			
				break;	

			//Range numbers
			case '[':
				//We've already done checking by now to make sure that this is actually valid
				if(*(cursor+1) == '0'){
					s = create_state(NUMBER, NULL, NULL);
					cursor += 4;
				} else if (*(cursor + 1) == 'a'){
					if(strlen(cursor) > 3 && *(cursor + 4) == 'A'){
						s = create_state(LETTERS, NULL, NULL);
						cursor += 7;
					} else {
						s = create_state(LOWERCASE, NULL, NULL);
						cursor += 4;
					}
				} else if (*(cursor + 1) == 'A'){
					s = create_state(UPPERCASE, NULL, NULL);
					cursor += 4;
				}

				//Concatenate to linked list
				if(tail == NULL){
					head = tail = s;
				} else {
					tail->next_created = s;
					tail = s;
				}

				//Create a fragment, with the fringe states of that fragment being just this new state that we
				//created
				fragment = create_fragment(s,  init_list(s));

				//Push the fragment onto the stack. We will pop it off when we reach operators
				push(stack, fragment);

				break;

			//Any character that is not one of the special characters
			default:
				//Create a new state with the charcter, and no attached states
				s = create_state(ch, NULL, NULL);

				//Concatenate to linked list
				if(tail == NULL){
					head = tail = s;
				} else {
					tail->next_created = s;
					tail = s;
				}

				//Create a fragment, with the fringe states of that fragment being just this new state that we
				//created
				fragment = create_fragment(s,  init_list(s));

				//Push the fragment onto the stack. We will pop it off when we reach operators
				push(stack, fragment);

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
	NFA_state_t* accepting_state = create_state(ACCEPTING, NULL, NULL);
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

	//Free the stack
	destroy_stack(stack, STATES_ONLY);

	NFA_state_t* cursor = starting_state;
	while(cursor != NULL){
		cursor = cursor->next_created;
	}

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
	} else if(start->opt == WILDCARD){
		//If we find a wildcard set this flag
		list->contains_wild_card = 1;
	} else if(start->opt == NUMBER){
		list->contains_numbers = 1;
	} else if(start->opt == LOWERCASE){
		list->contains_lowercase = 1;
	} else if(start->opt == UPPERCASE){
		list->contains_uppercase = 1;
	} else if(start->opt == LETTERS){
		list->contains_letters = 1;
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

	//Get all of the reachable NFA states for that DFA state, this is how we handle splits
	if(nfa_state != NULL){
		get_all_reachable_states(nfa_state, &(dfa_state->nfa_state_list));
	}

	//Return a pointer to our state
	return dfa_state;
}


/**
 * Make use of a variety of logic to properly make previous point to connecter
 */
void connect_DFA_states(DFA_state_t* previous, DFA_state_t* connecter){
	//We have numerous different cases to handle here
	
	//This means that we'll connect everything 
	if(connecter->nfa_state_list.contains_wild_card == 1){
		for(u_int16_t i = 0; i < 126; i++){
			previous->transitions[i] = connecter;
		}
	//If we have the '[0-9]' state
	} else if(connecter->nfa_state_list.contains_numbers == 1){
		for(u_int16_t i = '0'; i <= '9'; i++){
			previous->transitions[i] = connecter;
		}
	//If we have the '[a-z]' state
	} else if(connecter->nfa_state_list.contains_lowercase == 1){
		for(u_int16_t i = 'a'; i <= 'z'; i++){
			previous->transitions[i] = connecter;
		}

	//If we have '[A-Z]'
	} else if(connecter->nfa_state_list.contains_uppercase == 1){
		for(u_int16_t i = 'A'; i <= 'Z'; i++){
			previous->transitions[i] = connecter;
		}

	//If we have '[a-zA-Z]'
	} else if(connecter->nfa_state_list.contains_letters == 1){
		for(u_int16_t i = 'a'; i <= 'z'; i++){
			previous->transitions[i] = connecter;
		}

		for(u_int16_t i = 'A'; i <= 'Z'; i++){
			previous->transitions[i] = connecter;
		}
	//Otherwise we just have a regular state
	} else {
		for(u_int16_t i = 0; i < connecter->nfa_state_list.length; i++){
			u_int16_t opt = connecter->nfa_state_list.states[i]->opt;
			previous->transitions[opt] = connecter;
		}
	}
}


/**
 * Translate an NFA into an equivalent DFA using the reachability matrix
 * method. We will recursively figure out which states are reachable from other states. Our
 * new linked list of states should help us with this
 */
static DFA_state_t* create_DFA(NFA_state_t* nfa_start, regex_mode_t mode, u_int16_t flag_states, char go_until){
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
		if(nfa_cursor->visited == 3){
			nfa_cursor = nfa_cursor->next;
			continue;
		}

		if(nfa_cursor->opt == go_until){
			return dfa_start;
		}

		//We have different processing rules for each of our special cases. The default is of course that we just have a char
		switch(nfa_cursor->opt){
			case SPLIT_ZERO_OR_ONE:
				nfa_cursor->visited = 3;
				//Create the DFA for the straight path, marking each state as "off limits" whenever we see it.
				//This marking of off limits will ensure that the next function call will not retrace this
				//one's steps
				left_opt = create_DFA(nfa_cursor->next, mode, 0, '\0');

				//Create the right sub-DFA that is the optional "0 or 1" DFA
				right_opt = create_DFA(nfa_cursor->next_opt, mode, 0, nfa_cursor->next->opt);

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

				//Connect previous to left_opt
				connect_DFA_states(previous, left_opt);

				//Connect previous to right_opt
				connect_DFA_states(previous, right_opt);
				
				//Now we'll need to ensure that right_opt(At the very end) points to left_opt
				//Let's first advance to the very end of right_opt
				cursor = right_opt;
				//Advance to very end
				while(cursor->next != NULL){
					cursor = cursor->next;
				}

				//Connect the cursor to left_opt
				connect_DFA_states(cursor, left_opt);

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
				left_opt = create_DFA(nfa_cursor->next, mode, 0, '\0');
				right_opt = create_DFA(nfa_cursor->next_opt, mode, 0, '\0');

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
				
				//Connect previous to left_opt
				connect_DFA_states(previous, left_opt);

				//Connect previous to right_opt
				connect_DFA_states(previous, right_opt);

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
				left_opt = create_DFA(nfa_cursor->next, mode, 0, '\0');
				//Here is our actual "repeater"
				//IDEA -- go until we see the next guy's opt
				right_opt = create_DFA(nfa_cursor->next_opt, mode, 0, nfa_cursor->next->opt);


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
				//Connect previous to right_opt
				connect_DFA_states(previous, right_opt);

				//Connect previous to left_opt
				connect_DFA_states(previous, left_opt);

				//We'll now need to navigate to the end of the right opt repeater
				//sub-DFA
				cursor = right_opt;
				
				//Skip over
				while(cursor->next != NULL){
					cursor = cursor->next;
				}

				//Now that we're here, cursor holds the very end of the right sub-DFA
				//Everything that we have in the cursor must point back to the left DFA
				
				//Patching in right_opt
				//Everything that we have in the cursor must also point back to the right DFA
				connect_DFA_states(cursor, right_opt);

				//Connect cursor to left_opt
				connect_DFA_states(cursor, left_opt);
					
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
				left_opt = create_DFA(nfa_cursor->next, mode, 0, '\0');
				//Create the right DFA
				right_opt = create_DFA(nfa_cursor->next_opt, mode, 0, nfa_cursor->next->opt);
				

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
		
				//Connect previous to right_opt
				connect_DFA_states(previous, right_opt);

				//Connect previous to left_opt
				connect_DFA_states(previous,left_opt);
	
				//We'll now need to navigate to the end of the right opt repeater
				//sub-DFA
				cursor = right_opt;

				
				while(cursor->next != NULL){
					cursor = cursor->next;
				}

				//Now that we're here, cursor holds the very end of the right sub-DFA
				//Everything that we have in the cursor must point back to the left DFA
				
				//Everything that we have in the cursor must also point back to the right DFA
				//Connect cursor to right_opt
				connect_DFA_states(cursor, right_opt);

				//Connect cursor to left_opt
				connect_DFA_states(cursor, left_opt);
			
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

			default:
				//Create our state
				temp = create_DFA_state(nfa_cursor);
				//Connect previous to temp
				connect_DFA_states(previous, temp);
			
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
	regex_t* regex = calloc(1, sizeof(regex_t));
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

	//Create the NFA first
	regex->NFA = create_NFA(postfix, mode);

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
	regex->DFA = create_DFA(regex->NFA, mode, 0, '\0');

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

		if(current_state->nfa_state_list.contains_accepting_state == 1){
			//We've found the match
			match->status = MATCH_FOUND;

			if(mode == REGEX_VERBOSE){
				printf("Match found!\n");
			}

			return;

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
		}

		return;
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
void regex_match(regex_t* regex, regex_match_t* match_struct, char* string, u_int32_t starting_index, regex_mode_t mode){

	//Error mode by default
	match_struct->status = MATCH_ERR;

	//If we are given a bad regex 
	if(regex->DFA == NULL || regex->state == REGEX_ERR){
		//Verbose mode
		if(mode == REGEX_VERBOSE){
			printf("REGEX ERROR: Attempt to use an invalid regex.\n");
		}

		//Pack in the values and return match.match_start = 0;
		match_struct->match_start_idx = 0;
		match_struct->match_end_idx = 0;

		//We return this value so that the caller can know what the error was
		match_struct->status = MATCH_INV_INPUT;

		//Give the regex back
		return;
	}

	//If we are given a bad string
	if(string == NULL || strlen(string) == 0){
		//Verbose mode
		if(mode == REGEX_VERBOSE){
			printf("REGEX ERROR: Attempt to match a NULL string or a string of length 0.\n");
		}

		//Pack in the values and return
		match_struct->match_start_idx = 0;
		match_struct->match_end_idx = 0;
		match_struct->status = MATCH_INV_INPUT;
		return;
	}

	//Attempt to match the string with the regex
	match(match_struct, regex, string, starting_index, mode);

	//Return the match struct
	return;
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
