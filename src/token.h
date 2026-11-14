/**
 * @file  token.h
 * @brief Defines token types and token location information.
 */

#ifndef MSL_TOKEN_H
#define MSL_TOKEN_H

#include <stddef.h>

typedef enum {
    /* Delimiters */
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,

    /* Separators */
    TOKEN_COMMA, TOKEN_COLON,

    /* Keywords */
    TOKEN_METADATA, /* @METADATA */
    TOKEN_EPOCH,    /* @EPOCH    */
    TOKEN_ABSOLUTE, /* @ABSOLUTE */
    TOKEN_RELATIVE, /* @RELATIVE */
    TOKEN_TRUE, TOKEN_FALSE,

    /* Literals */
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER, /* Integers and floats                         */
    TOKEN_TIME,   /* Relative timestamps and absolute timestamps */

    TOKEN_EOF,
    TOKEN_TYPE_COUNT,
} TokenType;

typedef struct {
    TokenType type;
    const char* start; /* Lexeme literals tracked by location to save memory */
    size_t length;
    int line;
} Token;

#endif
