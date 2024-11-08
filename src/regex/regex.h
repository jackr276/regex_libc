/**
 * Author: Jack Robbins
 * This header file defines the API details for the regex library
 */

#ifndef REGEX_H
#define REGEX_H

#define ACCEPTING 129
#define NFA_END 150
//Split_T1 will never point back to itself
#define SPLIT_T1 127 
//Split T2 will point back to itself
#define SPLIT_T2 128 
#define START 0
#define REGEX_LEN 150
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
	//The state that the regex is in
	regex_state_t state;
	//The nuber of states in the regex
	u_int16_t num_states;

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
regex_t define_regular_expression(char* pattern, regex_mode_t mode);


/**
 * Determine whether or not a string belongs to the regular language defined by 
 * the DFA in regex_t.
 * Returns 0 if no match, 1 if a match
 */
regex_match_t regex_match(regex_t regex, char* string, u_int32_t starting_index, regex_mode_t mode);


/**
 * Deallocate all memory and destroy the regex passed in
 */
void destroy_regex(regex_t regex);

#endif
