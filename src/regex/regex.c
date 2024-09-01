/**
 * Author: Jack Robbins
 * This file contains the implementation for the regex library API defined in 
 * regex.h . Specifically, this file with procedurally generate a state machine that recognizes
 * strings belonging to a regular expression
 */

#include "regex.h" 
#include <iterator>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "../stack/stack.h"

#define ACCEPTING 257
#define SPLIT 256

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
	u_int32_t opt;
	//The default path
	state_t* path;
	//The optional second path
	state_t* path_opt;
};


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
 * Build an NFA for a regular expression defined by the pattern
 * passed in
 */
regex_t define_regular_expression(char* pattern){
	//Just in case
	if(pattern == NULL || strlen(pattern) == 0){
		printf("REGEX ERROR: Pattern cannot be null or empty");
		exit(1);
	}

	//Stack allocate a regex
	regex_t regex;

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
