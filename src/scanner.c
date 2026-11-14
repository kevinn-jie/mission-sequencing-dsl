/**
 * @file  scanner.c
 * @brief Scans source code into a list of tokens.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "scanner.h"

typedef struct {
    const char* source;
    TokenList* tokens;
    size_t start;
    size_t current;
    int line;
} Scanner;

static bool is_eof(const Scanner* scanner) {
    return scanner->source[scanner->current] == '\0';
}

static char advance(Scanner* scanner) {
    return scanner->source[scanner->current++];
}

static char peek(const Scanner* scanner) {
    return scanner->source[scanner->current];
}

static char peek_next(const Scanner* scanner) {
    if (is_eof(scanner)) return '\0';
    return scanner->source[scanner->current + 1];
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
}

static bool is_alpha_numeric(char c) {
    return is_alpha(c) || is_digit(c);
}

/**
 * @brief Appends a token spanning the current lexeme to the token list.
 *
 * @details Returns false and prints an error to stderr if the list cannot grow.
 *          On failure the existing list is left intact.
 */
static bool add_token(Scanner* scanner, TokenType type) {
    TokenList* list = scanner->tokens;

    if (list->count + 1 > list->capacity) {
        size_t new_capacity = list->capacity < 8 ? 8 : list->capacity * 2;
        Token* grown = realloc(list->tokens, new_capacity * sizeof(Token));
        if (grown == NULL) {
            fprintf(stderr, "error: not enough memory to store tokens.\n");
            return false;
        }

        list->tokens = grown;
        list->capacity = new_capacity;
    }

    Token* token = &list->tokens[list->count++];
    token->type = type;
    token->start = scanner->source + scanner->start;
    token->length = scanner->current - scanner->start;
    token->line = scanner->line;
    return true;
}

void scan_tokens(const char* source, TokenList* tokens) {
    Scanner scanner = {
        .source = source,
        .tokens = tokens,
        .start = 0,
        .current = 0,
        .line = 1,
    };

    /* TODO: while not at end: start = current; then scan one token */

    /* Zero-length lexeme since start is moved up to current */
    scanner.start = scanner.current;
    add_token(&scanner, TOKEN_EOF);
}
