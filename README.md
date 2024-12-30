# Efficient Regular Expression Creation & Matching with regex_libc
Author: [Jack Robbins](https://www.github.com/jackr276)

## Introduction
This project is a custom implementation of a regular expression matching library, using **Thompson's Construction**, also known as the **McNaughton–Yamada–Thompson algorithm** for regular expression to NFA conversion. Following this, a custom DFA creation algorithm is used. This has served both as a learning exercise for me, and as a tool that I will continue to use in other projects of my own. If other people wish to use or improve upon my project here, I welcome that and as such have licensed this under GPL 3.0. This README contains the documentation for the API an in-depth explanation of how the project works. In it's current implementation, we only support *, ?, (), | +, [a-z], $(wildcard), [0-9], [A-Z] and [a-zA-Z]. I am hoping to continue to build atop it as needed.

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

Once done, this NFA will have **as many states as the regular expression has characters**. There is no optimization that occurs of any kind, meaning that inefficient or overly complicated regular expressions will become inefficient and overly complicated NFAs.

Here is a renditition of the NFA that will be created with this particular regular expression:   

![NFA drawio](https://github.com/user-attachments/assets/84609723-cb85-4d4b-8dfc-d822ed0c344c)



### Step 4: Converting the NFA into an equivalent DFA


