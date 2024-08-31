/**
 * Author: Jack Robbins
 * This header file defines the API details for the regex library
 */

#ifndef REGEX_H
#define REGEX_H



/**
 * A struct that contains all information needed for a regular expression
 */
typedef struct {
	//The regex we wish to match with
	char* regex;
	//The pointer to the DFA, the user should never touch this
	void* DFA;
} regex_t;


/**
 * Define a regular expression using all regular expression rules
 */
regex_t define_regular_expression(char* pattern);


/**
 * Determine whether or not a string belongs to the regular language defined by 
 * the DFA in regex_t.
 * Returns 0 if no match, 1 if a match
 */
int regex_match(regex_t regex, char* string);

#endif
