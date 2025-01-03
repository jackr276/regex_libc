/**
 * Author: Jack Robbins
 * This header file defines the API details for the regex library
 */

#ifndef REGEX_H
#define REGEX_H

//The accepting constant
#define ACCEPTING 132
//Split_one_or_more is specifically used for the question mark(?) operator
#define SPLIT_ZERO_OR_ONE 128 
//Split_alternate is specifically used for the alternation(|) operator
#define SPLIT_ALTERNATE 129
//Split Kleene is specifically used for the kleene star(*) operator
#define SPLIT_KLEENE 130 
//Split_Pos_clos is specifically used for the positive closure(+) operator
#define SPLIT_POSITIVE_CLOSURE 131
//Define a wildcard
#define WILDCARD 133
//Define number 0-9
#define NUMBER 134
//Define our a-z range
#define LOWERCASE 135
//Define our A-Z range
#define UPPERCASE 136
//Define a-zA-Z
#define LETTERS 137
//The maximum length of a regex is 150
#define REGEX_LEN 150
//This is the explicit concatenation character. It is used in place of actual concatenation
#define CONCATENATION '`'

#include <stdint.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/types.h>
#include "../stack/stack.h"

/**
 * The state that the regex is in. Used for an "errors as values" return approach
 */
typedef enum {
	REGEX_ERR,
	REGEX_VALID,
} regex_state_t;


/**
 * The status of a match call. Match calls can return various errors or signals
 * letting the caller know if a match was found
 */
typedef enum {
	MATCH_INV_INPUT,
	MATCH_ERR,
	MATCH_FOUND,
	MATCH_NOT_FOUND,
} match_status_t;


/**
 * For testing purposes, the state machine generator and match function can
 * be put into "VERBOSE" mode to print out all erros encountered. "SILENT" mode
 * is recommended for all other uses as it will remove all printf commands
 */
typedef enum {
	REGEX_VERBOSE,
	REGEX_SILENT,
} regex_mode_t;


/**
 * A struct that contains all information needed for a regular expression
 */
typedef struct {
	//The regex we wish to match with
	char* regex;
	//The pointer to the NFA, the user should never touch this
	void* NFA;
	//The pointer to the DFA, the user should also never touch this
	void* DFA;
	//The creation chain for the NFA
	void* creation_chain;
	//The state that the regex is in
	regex_state_t state;
} regex_t;


/**
 * A return type struct that allows for value packing by regex_match()
 */
typedef struct {
	//The pointer to the first match index
	u_int32_t match_start_idx;
	//The pointer to the end of the match
	u_int32_t match_end_idx;
	//The match status
	match_status_t status;

} regex_match_t;


/**
 * Define a regular expression using all regular expression rules
 */
regex_t* define_regular_expression(char* pattern, regex_mode_t mode);


/**
 * Determine whether or not a string belongs to the regular language defined by 
 * the DFA in regex_t.
 * Returns 0 if no match, 1 if a match
 */
void regex_match(regex_t* regex, regex_match_t* match_struct, char* string, u_int32_t starting_index, regex_mode_t mode);


/**
 * Deallocate all memory and destroy the regex passed in
 */
void destroy_regex(regex_t* regex);

#endif
