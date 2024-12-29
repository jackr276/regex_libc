# Efficient Regular Expression Creation & Matching with regex_libc
Author: [Jack Robbins](https://www.github.com/jackr276)

## Introduction
This project is a custom implementation of a regular expression matching library, using **Thompson's Construction**, also known as the **McNaughton–Yamada–Thompson algorithm** for regular expression to NFA conversion. Following this, a custom DFA creation algorithm is used. This has served both as a learning exercise for me, and as a tool that I will continue to use in other projects of my own. If other people wish to use or improve upon my project here, I welcome that and as such have licensed this under GPL 3.0. This README contains the documentation for the API an in-depth explanation of how the project works. In it's current implementation, we only support *, ?, (), | +, [a-z], $(wildcard), [0-9], [A-Z] and [a-zA-Z]. I am hoping to continue to build atop it as needed.

## Contents
* [API Details](https://github.com/jackr276/regex_libc?tab=readme-ov-file#api-details)
* [Using a regex](https://github.com/jackr276/regex_libc?tab=readme-ov-file#2-using-a-regex)
* [Recognized Operators]()
* [Example Usage](https://github.com/jackr276/regex_libc?tab=readme-ov-file#example-usage)

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



