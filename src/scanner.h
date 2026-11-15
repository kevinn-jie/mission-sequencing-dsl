/**
 * @file  scanner.h
 * @brief Allows scanner.c to be public interface.
 */

#ifndef MSL_SCANNER_H
#define MSL_SCANNER_H

#include <stdbool.h>
#include <stddef.h>

#include "token.h"

/* Expandable list of tokens scanner appends to */
typedef struct {
    Token* tokens;
    size_t count;
    size_t capacity;
} TokenList;

/**
 * @brief   Scans a null-terminated source string into a list of tokens.
 * @details Tokens are appended to `tokens`, which the caller owns.
 *          Every lexical error is printed to stderr with its line,
 *          and scanning continues so that all errors are reported.
 *          Returns false if any error occurred.
 */
bool scan_tokens(const char* source, TokenList* tokens);

#endif
