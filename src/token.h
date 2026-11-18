/**
 * @file  token.h
 * @brief Defines token types and token location information.
 */

#ifndef MSL_TOKEN_H
#define MSL_TOKEN_H

#include <stddef.h>

typedef enum {
    /* Delimiters */
    TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,

    /* Separators */
    TOKEN_COMMA, TOKEN_COLON,

    /* Keywords  */
    /* @METADATA */
    TOKEN_METADATA,
    /* @EPOCH */
    TOKEN_EPOCH,
    /* @ABSOLUTE */
    TOKEN_ABSOLUTE,
    /* @RELATIVE */
    TOKEN_RELATIVE,
    TOKEN_TRUE, TOKEN_FALSE,

    /* Literals */
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    /* Integers and floats */
    TOKEN_NUMBER,
    /* Relative timestamps and absolute timestamps */
    TOKEN_TIME,

    TOKEN_EOF,
    TOKEN_TYPE_COUNT,
} TokenType;

typedef struct {
    TokenType type;

    /* Lexeme literals tracked by location to save memory */
    const char* start;
    size_t length;
    int line;
} Token;

#endif
