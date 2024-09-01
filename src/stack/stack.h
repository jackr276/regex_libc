/**
 * Author: Jack Robbins
 * An API for a stack implementation
 */

#ifndef STACK_H
#define STACK_H

#include <sys/types.h>

//Allows us to use stack_node_t as a type
typedef struct stack_node_t stack_node_t;

/**
 * Nodes for our stack
 */
struct stack_node_t {
	stack_node_t* next;
	void* data;
};


/**
 * A reference to the the stack object that allows us to
 * have more than one stack
 */
typedef struct {
	struct stack_node_t* top;
	u_int16_t num_nodes;
} stack_t;


/**
 * Initialize a stack
 */
stack_t* create_stack();

/**
 * Push a pointer onto the top of the stack
 */
void push(stack_t* stack, void* element);

/**
 * Remove the top value of the stack
 */
void* pop(stack_t* stack);

/**
 * Return the top value of the stack, but do not
 * remove it
 */
void* peek(stack_t* stack);

/**
 * Destroy the stack with a proper cleanup
 */
void destroy_stack(stack_t* stack);

#endif
