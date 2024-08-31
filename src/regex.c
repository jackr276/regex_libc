/**
 * Author: Jack Robbins
 * This file contains the implementation for the regex library API defined in 
 * regex.h . Specifically, this file with procedurally generate a state machine that recognizes
 * strings belonging to a regular expression
 */

#include "regex.h" 

typedef struct {
	int is_end;
	
} state_t;


/**
 * A struct that contains everything needed for the dfa that defines
 * the regular expression
 */
typedef struct {

} automaton_t;



//STUB
regex_t define_regular_expression(char *pattern){
	regex_t regex;

	return regex;
}

//STUB
int regex_match(regex_t regex, char *string){
	return 0;
}


