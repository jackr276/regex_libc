# Efficient Regular Expression Creation & Matching with regex_libc
Author: [Jack Robbins](https://www.github.com/jackr276)

## Introduction
This project is a custom implementation of a regular expression matching tool, using **Thompson's Construction**, also known as the **McNaughton–Yamada–Thompson algorithm** for regular expression to NFA conversion. Following this, a custom DFA creation algorithm is used. This has served both as a learning exercise for me, and as a tool that I will continue to use in other projects of my own. If other people wish to use or improve upon my project here, I welcome that and as such have licensed this under GPL 3.0. This README contains the documentation for the API an in-depth explanation of how the project works. In it's current implementation, we only support *, ?, (), | +, [a-z], $(wildcard), [0-9], [A-Z] and [a-zA-Z]. I am hoping to continue to build atop it as needed.

## API Details
There are really on 3 main functions that are exposed to the user via the `regex.h` header file. They are as follows:  
### 1.) Creating a regex
```C
regex_t* define_regular_expression(char* pattern, regex_mode_t mode)
```
This will return a reference to a created `regex_t` struct. This struct contains information necessary to the internal functioning of the system, but it itself should not be accessed or modified by the user. The `regex_mode_t` allows the user to specify the logging levels of the regex. It is recommended that all users use `REGEX_SILENT` as the mode, as `REGEX_VERBOSE` is set to a very high logging level. If creation of the regex is successful, the following paramater will be set like this: `regex->state = REGEX_VALID`. If it is not valid, the state will be set as `regex->state = REGEX_ERROR`. If the regex struct is valid, the user can use it for as long as they please.

### 2.) Using a regex
```C
void regex_match(regex_t* regex, regex_match_t* match_struct, char* string, u_int32_t starting_index, regex_mode_t mode)
```
This function will start searching through `string` at the specified `starting_index`. The `mode` has the same options for creating a regex and the above recommendation of only using silent mode stands.    
The `regex_match` struct is as follows:
```C
typedef struct {
	//The pointer to the first match index
	u_int32_t match_start_idx;
	//The pointer to the end of the match
	u_int32_t match_end_idx;
	//The match status
	match_status_t status;

} regex_match_t;

typedef enum {
	MATCH_INV_INPUT,
	MATCH_ERR,
	MATCH_FOUND,
	MATCH_NOT_FOUND,
} match_status_t;
```
Note that the match function returns the very first match found. Subsequent matches require the user to advance the `starting_index` to where they wish to start. The `match_status_t` enumerated type is in my opinion self explanatory, so I will not detail it further.

>[!NOTE]
>The `match_end_idx` is exclusive. So for example, if the match struct returns `match_start_idx = 0` and `match_end_idx = 9`, that means that the match starts at 0 and goes up to **but does not include** index 9

### 3.) Cleaning up a regex
```C
void destroy_regex(regex_t* regex)
```
Since regex structs are dynamically allocated and contain many dynamically allocated parts, a cleanup function is needeed. As the user, you only need to pass in the reference to the regex struct to this function. The function will deallocate all memory. `regex_libc` is completely memory safe, so this cleanup function will avoid any/all memory leaks.

## Recognized Operators
This library supports all printable characters(ASCII range 32-126). Some printable characters have been reserved for other functions and as such the escape character `\` is required to be used in front of them. Here is a detailed list of all operators:
|Operator|Operator Character(s)|Functionality|
|--------|----------|-------------|
|Kleene star|*|Repeat the preceeding character 0 or many times|
|Positive Closure|+|Repeat the preceeding chunk 1 or many times|
|Optionality  |?|The preceeding chunk may be seen 0 or 1 times|
|Alternation|\||The chunk on the left and right side may be seen interchangeably|
|Number range|[0-9]|Any digit(0-9) may be seen|
|Lowercase range|[a-z]|Any lowercase character may be seen|
|Uppercase range|[A-Z]|Any uppercase character may be seen|
|Letter range|[a-zA-Z]|Any letter may be seen regardless of case|
|Grouping||()|Group the characters inside of the parenthesis|
|Wildcard|$|Any character may be seen|
|Explicit Concatenation|\`|**INTERNAL USE ONLY**. The user should never attempt to put these in themselves|

For all of the operators above, if you wish to actually find these operators in the string, you must use the escape character `\`. If you use this, the normally special character will be treated like a normal character.

### A note on parenthesization
This library supports-and heavily encourages-the use of parenthesis in the regular expressions given. Lack of parenthesis may lead to unexpected results, as the system takes a shortest-string grouping approach.

For example, `abc|def` will be interpreted as "a then b then c or d then e then f". A lot of the time, people actually want to have "abc OR def", in which case you would use the following parenthesization: `(abc)|(def)`. There is no restriction on how heavily you can parenthesize, so when in doubt use parenthesis.


## Example Usage:
Here is a quick example of using this regex library to find all filenames that end in ".txt". 

```C
regex_match_t matcher;
tester = define_regular_expression("$+.txt", REGEX_SILENT);

//Define a test string
test_string = "fname.txt";
			
//Test the matching
regex_match(tester, &matcher, test_string, 0, REGEX_SILENT);

//Display if we've found a match
if(matcher.status == MATCH_FOUND){
	printf("Match starts at index: %d and ends at index:%d\n", matcher.match_start_idx, matcher.match_end_idx);
} else {
	printf("No match.\n\n");
}

//Destroy the regex
destroy_regex(tester)
```
This will display the following output in `REGEX_SILENT` mode:
```Console
Match starts at index: 0 and ends at index:9
```

In `REGEX_VERBOSE` mode, the following will be shown:
```Console
With concatenation characters added: $+`.`t`x`t
Postfix regular expression: $+.`t`x`t`
Postfix conversion: $+.`t`x`t`

NFA conversion succeeded.
State -->State -SPLIT_POSITIVE_CLOSURE->State -.->State -t->State -x->State -t->State -ACCEPTING->
State -->

Beginning DFA Conversion.

DFA conversion succeeded.
regex_t creation succeeded. Regex is now ready to be used.
TEST STRING: fname.txt

Pattern continued/started with character: f
Pattern continued/started with character: n
Pattern continued/started with character: a
Pattern continued/started with character: m
Pattern continued/started with character: e
Pattern continued/started with character: .
Pattern continued/started with character: t
Pattern continued/started with character: x
Pattern continued/started with character: t
Match found!
Match starts at index: 0 and ends at index:9
```

There are many more example usages contained in the testing file: [regex_testing.c](https://github.com/jackr276/regex_libc/blob/main/src/regex_testing.c). There are 76 test cases, pretty much encompassing most of what you'd want to do with a regular expression library like this(email matching, filename matching, etc.). This concludes the API details for the library, so if you simply wish to use the library, then you can stop reading here. It is my assumption that anyone who is in need of a library as specialized as this would be able to link it properly, so I will not detail that here. Following this is the techincal description of how the library actually works.

## Technical Description
Let's now look at how the system works. There are 4 fundamental parts to regular expression creation used by this library, and the steps are as follows:
1. The regular expression entered by the user has **explicit concatenation** characters(`) added in appropriate areas
2. The regular expression is converted into a **postfix** expression using the Shunting-Yard algorithm
3. The NFA is constructed using the **McNaughton-Yamada-Thompson** algorithm, with some important tweaks
4. This NFA is converted into a DFA using **my novel algorithm**. After this, the regex is ready to begin matching strings

Let's now look at each step indvidually.

### Step 1: Adding explicit concatenation characters
The first and arguably simplest step is the addition of explicit concatenation characters. I've decided to use the \` as my explicit concatenation character as it is rarely used regularly, but the choice of character is irrelevant to functionality. The system will take in a regular expression provided by the user, say for example: (\$*)@(\$*).((com)|(edu)) and add in explicit concatenation characters where needed. The result will look like this: ($*)\`@\`($*)\`.\`((c\`o\`m)|(e\`d\`u)). These explicit characters allow us to know exactly which fragments must be connected to one another. The rules for adding these explicit concatenation characters are well commented in the code, and as such I will not review them here.

### Step 2: Converting the regular expression into postfix form
Next we use the **Shunting Yard** algorithm to convert the regular expression into postfix form. This [article](https://blog.cernera.me/converting-regular-expressions-to-postfix-notation-with-the-shunting-yard-algorithm/) does an excellent job explaining and visualizing the procedure, and I would encourage anyone interested to read it. Again this process is very faithful to the original shunting yard algorithm, with no real deviations, so I will leave any further exploration to the reader.

Using our previous example, the infix expression: ($*)\`@\`($*)\`.\`((c\`o\`m)|(e\`d\`u)) will be converted into: $*@\`$*\`.\`co\`m\`ed\`u\`|\` . Once we have it in this form, our work on converting to an NFA can truly begin.

### Step 3: Converting the postfix regular expression into an equivalent NFA
The **McNaughton-Yamada-Thompson** process relies on the use of NFA "fragments" and a stack. The core idea is simple: as we encounter regular characters, we create NFA fragments with a single state and push them onto the stack. When we encounter an operator, we will pop the first one or two(depending on the operator) most recent fragments off of the stack and combine them appropriately with a special kind of "split" state. This new fragment is then pushed back onto the stack. In essence, we are creating mini-NFAs for each process and then combining them when we see operators. The modifications that have been made to the algorithm in this project are as follows:  
1. The use of different "split" states: SPLIT_KLEENE, SPLIT_ALTERNATE, SPLIT_POSITIVE_CLOSURE, SPLIT_ZERO_OR_ONE. This allows us to take special action when we see these states
2. Each NFA state contains a "next_created" state. This is done for the purposes of memory management. It would be impossible to free the memory if we didn't have this, because many of these states are self referential.

Once done, this NFA will have **as many states as the regular expression has characters**. There is no optimization that occurs of any kind, meaning that inefficient or overly complicated regular expressions will become inefficient and overly complicated NFAs. Each NFA state is only allowed to have **two transitions**. This greatly simplifies creation and avoids any headaches with having a variable number of transitions. Regular states will only use one transition, whilst split states do make use of the two.

Here is a renditition of the NFA that will be created with this particular regular expression:   

![NFA drawio](https://github.com/user-attachments/assets/84609723-cb85-4d4b-8dfc-d822ed0c344c)



### Step 4: Converting the NFA into an equivalent DFA
For our purposes, the NFA is only an intermediate step. We do not want to use the NFA to perform matching because it is **non-deterministic**, meaning that we'd likely have to try many separate paths before determining if we have a match or not. Luckily, every NFA has an equivalent DFA, and those are deterministic. So, all that we need to do now is create the equivalent DFA for our NFA and we should be good to go. My implementation is somewhat like Sipser's table implementation, but there are some differences that make it more programmatically sound. For some context, here are the two C structs that are used in the DFA:
```C
/**
 * A state that we will use for our DFA caching scheme. It is well known that DFA's are more efficent
 * than NFAs, so this will help us speed things up
 */
struct DFA_state_t {
	//The list of the NFA states that make up the DFA state
	NFA_state_list_t nfa_state_list;
	//Hold the address of the NFA state
	NFA_state_t* nfa_state;
	//This list is a list of all the states that come from this DFA state. We will use the char itself to index this state. Remember that printable
	//chars range from 0-144
	DFA_state_t* transitions[135];
	//The next dfa_state that was made, this will help us in freeing
	DFA_state_t* next;
};

/**
 * When converting from an NFA to a DFA, we represent each DFA state as a list of reachable
 * NFA states. This struct will be used for this purpose
 */
struct NFA_state_list_t {
	NFA_state_t* states[145];
	//Length of our list
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
```
Let's parse these structs. DFA_state_t first holds something of type NFA_state_list_t. The NFA_state_list_t holds all of the NFA states that this DFA state makes up. If you review Sipser's formula for conversion of NFAs to DFAs, you will see that DFA states are made up of 1 or many NFA states that can be **reached** from that DFA state. This is what this here intends to mimic. There is also a pointer to another NFA_state_t. This is there simply to help with DFA_state equality and acts as an efficiency speedup. If we want to compare to DFA states, we can first start by comparing the memory addresses of the NFA states that they were both derived from. If this fails, we can then compare their NFA_state_lists for equality. This kind of comparison is needed for the implementation of **positive closure(+)**. There is also an array of 135 DFA_state_t* pointers. This is where the real efficiency happens. We **encode** the transition char as an index in this array. This allows us to match very quickly. For example, if DFA_state_t* state1 should go to state2 when we see char 'a'(ASCII 97), then state1->transitions[97] = state2. We need 135 as opposed to 128 because there are some special states like ACCEPTING(131) that have encodings above the normal ASCII range for obvious reasons. Note that we have several boolean(u_int8_t) values inside of the NFA_state_list_t. This is simply there to make it easier for us to handle special characters like wildcards($) and others, as opposed to having to search through the state list every single time.

To create the DFA, we go through state by state in the NFA. For any "SPLIT" states that we encounter, we will mark them as being "visited". This ensures that we don't have an infinite loop of state creation. States like SPLIT_ALTERNATE, SPLIT_ZERO_OR_ONE and SPLIT_KLEENE will trigger a recursive call that recursively builds out their left and right hand NFA connections as DFAs themselves. SPLIT_POSITIVE_CLOSURE is a special case, because a positive closure state always comes after the states that need to be repeated. As such, only one recursive call is made for the positive closure state. For regular states, a simple creation takes place that forms the base case of our whole operation. That is as follows:
```C
//Create our state
temp = create_DFA_state(nfa_cursor, chain, next_idx);
//Connect previous to temp
connect_DFA_states(previous, temp);

//Advance the current DFA pointer
previous->next = temp;
previous = temp;

//Advance the pointer
nfa_cursor = nfa_cursor->next;
break;

//================================DFA state function================================
static DFA_state_t* create_DFA_state(NFA_state_t* nfa_state, DFA_state_t** chain, u_int16_t* next_idx){
	//Allocate a DFA state
	DFA_state_t* dfa_state = (DFA_state_t*)calloc(1, sizeof(DFA_state_t));
	dfa_state->nfa_state = nfa_state;
	//Add this in
	chain[*next_idx] = dfa_state;
	//Increment
	(*next_idx)++;

	//Set to null as warning
	dfa_state->next = NULL;

	//Get all of the reachable NFA states for that DFA state, this is how we handle splits
	if(nfa_state != NULL){
		get_all_reachable_states(nfa_state, &(dfa_state->nfa_state_list));
	}

	//Return a pointer to our state
	return dfa_state;
}

//=============================Reachable function===================================
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
	list->states[list->length] = start;
	list->length++;

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
```
This is how the base case DFA state creation is handled. In terms of the connections, there is a special connect_DFA_states function that will use the trick that we mentioned earlier to connect two states using their transitions[]. I've included the function here:
```C
/**
 * Make use of a variety of logic to properly make previous point to connecter
 */
void connect_DFA_states(DFA_state_t* previous, DFA_state_t* connecter){
	//We have numerous different cases to handle here
	
	//This means that we'll connect everything 
	if(connecter->nfa_state_list.contains_wild_card == 1){
		for(u_int16_t i = 32; i < 127; i++){
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
```

This should give you a basic idea of how the DFA creation algorithm works. For a full view of how it works, please view the source code here: [regex.c](https://github.com/jackr276/regex_libc/blob/main/src/regex/regex.c).

## Future Work
This project is in a working state as of right now, passing all test cases that I can think of giving it. THere are of course many more features that I could add, say for instance anchoring, but I feel like the fundamentals are already here in this library. If you find any issues, I would encourage you to open an issue [here](https://github.com/jackr276/regex_libc/issues) and I will be sure to take a look. This has been a wonderful learning experience for me and I hope that it will work as well as it does for me as a regular expression matching tool.
