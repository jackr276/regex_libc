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
	char* regex;
	// TODO add more
} regex_t;

regex_t define_regular_expression(char* regex);

#endif
 

