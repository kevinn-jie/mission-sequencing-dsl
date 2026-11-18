/**
 * @file  parser.h
 * @brief Allows parser.c to be public interface.
 */

#ifndef MSL_PARSER_H
#define MSL_PARSER_H

#include <stdbool.h>
#include <stddef.h>

#include "scanner.h"
#include "token.h"

typedef struct Value Value;
typedef struct Entry Entry;

/* Expandable list of values (array elements and step arguments) */
typedef struct {
    Value* items;
    size_t count;
    size_t capacity;
} ValueList;

/* Expandable list of key-value entries (object entries and metadata headers) */
typedef struct {
    Entry* items;
    size_t count;
    size_t capacity;
} EntryList;

typedef enum {
    VALUE_STRING,
    VALUE_NUMBER,
    VALUE_BOOLEAN,
    VALUE_ARRAY,
    VALUE_OBJECT,
} ValueType;

struct Value {
    ValueType type;

    /* The literal itself, or the opening "[" or "{" */
    Token token;
    union {
        bool boolean;
        ValueList array;
        EntryList object;
    } as;
};

/* Key is a STRING token for metadata headers, an IDENTIFIER token for objects */
struct Entry {
    Token key;
    Value value;
};

typedef struct {
    /* @EPOCH, @ABSOLUTE, or @RELATIVE */
    Token time_frame;
    Token time;
    Token stem;
    ValueList args;
} Step;

/* Expandable list of steps */
typedef struct {
    Step* items;
    size_t count;
    size_t capacity;
} StepList;

typedef struct {
    EntryList metadata;
    StepList steps;
} Sequence;

/**
 * @brief   Parses a scanned token list into a sequence.
 * @details tokens must end with a TOKEN_EOF token.
 *          Every syntax error is printed to stderr with its line,
 *          and parsing resumes at the next metadata header or step so that all errors are reported.
 *          Returns false if any error occurred.
 *          Tokens in sequence point into the source string,
 *          so the source must outlive sequence.
 *          The caller must call free_sequence() on sequence whether parsing succeeded or not.
 */
bool parse_sequence(const TokenList* tokens, Sequence* sequence);

/**
 * @brief Frees every list owned by sequence and leaves it empty.
 */
void free_sequence(Sequence* sequence);

#endif
