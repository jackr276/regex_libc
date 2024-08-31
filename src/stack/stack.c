/**
 * Author: Jack Robbins
 * The implementation of the stack functions defined by the API in stack.h
 */


/**
 * Nodes for our stack
 */
struct stack_node {
	struct stack_node* next;
	void* data;
};



