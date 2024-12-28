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

### 3.) Cleaning up a regex
```C
void destroy_regex(regex_t* regex)
```
Since regex structs are dynamically allocated and contain many dynamically allocated parts, a cleanup function is needeed. As the user, you only need to pass in the reference to the regex struct to this function. The function will deallocate all memory. `regex_libc` is completely memory safe, so this cleanup function will avoid any/all memory leaks.
