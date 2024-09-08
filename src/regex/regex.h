/**
 * Author: Jack Robbins
 * This header file defines the API details for the regex library
 */

#ifndef REGEX_H
#define REGEX_H

#include <stdint.h>

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
	//The pointer to the DFA, the user should never touch this
	void* DFA;
	//The state that the regex is in
	regex_state_t state;

} regex_t;


/**
 * A return type struct that allows for value packing by regex_match()
 */
typedef struct {
	//The pointer to the first match instance
	char* match;
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
regex_match_t regex_match(regex_t regex, char* string, regex_mode_t mode);

#endif
