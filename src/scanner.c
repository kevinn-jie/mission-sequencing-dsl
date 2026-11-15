/**
 * @file  scanner.c
 * @brief Scans source code into a list of tokens.
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scanner.h"

typedef struct {
    const char* source;
    TokenList* tokens;
    size_t start;
    size_t current;
    int line;
    bool had_error;
    bool out_of_memory;
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
    if (is_eof(scanner)) {
        return '\0';
    }

    return scanner->source[scanner->current + 1];
}

static bool is_digit(char character) {
    return character >= '0' && character <= '9';
}

static bool is_alpha(char character) {
    return (character >= 'a' && character <= 'z') ||
           (character >= 'A' && character <= 'Z') ||
            character == '_';
}

static bool is_alpha_numeric(char character) {
    return is_alpha(character) || is_digit(character);
}

static bool is_hex_digit(char character) {
    return is_digit(character) ||
           (character >= 'a' && character <= 'f') ||
           (character >= 'A' && character <= 'F');
}

static void error(Scanner* scanner, const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "error: line %d: ", scanner->line);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);
    va_end(args);
    scanner->had_error = true;
}

/**
 * @brief   Appends a token spanning the current lexeme to the token list.
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
            scanner->had_error = true;
            scanner->out_of_memory = true;
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

static size_t lexeme_length(const Scanner* scanner) {
    return scanner->current - scanner->start;
}

static const char* lexeme_text(const Scanner* scanner) {
    return scanner->source + scanner->start;
}

static bool lexeme_is(const Scanner* scanner, const char* text) {
    size_t length = lexeme_length(scanner);
    return length == strlen(text) && memcmp(lexeme_text(scanner), text, length) == 0;
}

static bool lexeme_matches(const Scanner* scanner, const char* pattern) {
    size_t length = lexeme_length(scanner);
    if (length != strlen(pattern)) {
        return false;
    }

    const char* text = lexeme_text(scanner);
    for (size_t i = 0; i < length; i++) {
        bool fits = pattern[i] == 'd' ? is_digit(text[i]) : text[i] == pattern[i];
        if (!fits) {
            return false;
        }
    }

    return true;
}

/* Reports whether the current lexeme is a JSON number: -?(0|[1-9]d*)(.d+)?([eE][+-]?d+)? */
static bool lexeme_is_number(const Scanner* scanner) {
    const char* text = lexeme_text(scanner);
    const char* end = text + lexeme_length(scanner);

    if (text < end && *text == '-') {
        text++;
    }

    if (text == end || !is_digit(*text)) {
        return false;
    }

    if (*text == '0') {
        text++;
    }

    else {
        while (text < end && is_digit(*text)) {
            text++;
        }
    }

    /* Optional fraction */
    if (text < end && *text == '.') {
        text++;
        if (text == end || !is_digit(*text)) {
            return false;
        }

        while (text < end && is_digit(*text)) {
            text++;
        }
    }

    /* Optional exponent */
    if (text < end && (*text == 'e' || *text == 'E')) {
        text++;
        if (text < end && (*text == '+' || *text == '-')) {
            text++;
        }

        if (text == end || !is_digit(*text)) {
            return false;
        }

        while (text < end && is_digit(*text)) {
            text++;
        }
    }

    return text == end;
}

/**
 * @brief   Consumes the escape code after a backslash in a string.
 * @details Accepts the JSON escapes \" \\ \/ \b \f \n \r \t and \uXXXX.
 *          Returns false and reports an error for anything else.
 */
static bool scan_escape(Scanner* scanner) {
    switch (peek(scanner)) {
        case '"': case '\\': case '/':
        case 'b': case 'f': case 'n': case 'r': case 't':
            advance(scanner);
            return true;

        case 'u':
            advance(scanner);
            for (int i = 0; i < 4; i++) {
                if (!is_hex_digit(peek(scanner))) {
                    error(scanner, "Invalid \\u escape: expected 4 hex digits.");
                    return false;
                }

                advance(scanner);
            }

            return true;

        default:
            /* Not consumed so newline or end of input still ends string */
            error(scanner, "Invalid escape sequence in string.");
            return false;
    }
}

/* Scans a one-line string; token keeps quotes and raw escapes */
static void scan_string(Scanner* scanner) {
    bool valid = true;

    while (peek(scanner) != '"' && peek(scanner) != '\n' && !is_eof(scanner)) {
        if (advance(scanner) == '\\' && !scan_escape(scanner)) {
            valid = false;
        }
    }

    if (peek(scanner) != '"') {
        error(scanner, "Unterminated string.");
        return;
    }

    /* Closing quote */
    advance(scanner);

    if (valid) {
        add_token(scanner, TOKEN_STRING);
    }
}

static void scan_keyword(Scanner* scanner) {
    while (is_alpha_numeric(peek(scanner))) {
        advance(scanner);
    }

    if (lexeme_is(scanner, "@METADATA")) {
        add_token(scanner, TOKEN_METADATA);
    }
    
    else if (lexeme_is(scanner, "@EPOCH")) {
        add_token(scanner, TOKEN_EPOCH);
    }
    
    else if (lexeme_is(scanner, "@ABSOLUTE")) {
        add_token(scanner, TOKEN_ABSOLUTE);
    }
    
    else if (lexeme_is(scanner, "@RELATIVE")) {
        add_token(scanner, TOKEN_RELATIVE);
    }
    
    else if (lexeme_length(scanner) == 1) {
        error(scanner, "Expected a keyword name after '@'.");
    }
    
    else {
        error(scanner, "Unknown keyword '%.*s'.", (int)lexeme_length(scanner), lexeme_text(scanner));
    }
}

static void scan_identifier(Scanner* scanner) {
    while (is_alpha_numeric(peek(scanner))) {
        advance(scanner);
    }

    if (lexeme_is(scanner, "true")) {
        add_token(scanner, TOKEN_TRUE);
    }
    
    else if (lexeme_is(scanner, "false")) {
        add_token(scanner, TOKEN_FALSE);
    }
    
    else {
        add_token(scanner, TOKEN_IDENTIFIER);
    }
}

/* Separators inside numbers and timestamps are only consumed when a digit follows */
static bool continues_number_or_time(const Scanner* scanner) {
    char next = peek(scanner);
    if (is_alpha_numeric(next)) {
        return true;
    }

    bool is_separator = next == '.' || next == ':' || next == '-' || next == '+';
    return is_separator && is_digit(peek_next(scanner));
}

/**
 * @brief   Scans a number or timestamp, then checks its whole lexeme against the strict formats.
 * @details Absolute timestamps are YYYY-DDDTHH:MM:SS.sss,
 *          relative timestamps are HH:MM:SS.sss, and numbers follow the JSON number format.
 */
static void scan_number_or_time(Scanner* scanner) {
    while (continues_number_or_time(scanner)) {
        advance(scanner);
    }

    if (lexeme_matches(scanner, "dddd-dddTdd:dd:dd.ddd") ||
        lexeme_matches(scanner, "dd:dd:dd.ddd")) {
        add_token(scanner, TOKEN_TIME);
    }
    
    else if (lexeme_is_number(scanner)) {
        add_token(scanner, TOKEN_NUMBER);
    }
    
    else {
        error(scanner, "Malformed number or timestamp '%.*s'.",
              (int)lexeme_length(scanner), lexeme_text(scanner));
    }
}

static void scan_token(Scanner* scanner) {
    char character = advance(scanner);

    switch (character) {
        case '[': 
            add_token(scanner, TOKEN_LEFT_BRACKET); break;

        case ']': 
            add_token(scanner, TOKEN_RIGHT_BRACKET); break;

        case '{': 
            add_token(scanner, TOKEN_LEFT_BRACE); break;

        case '}': 
            add_token(scanner, TOKEN_RIGHT_BRACE); break;

        case ',': 
            add_token(scanner, TOKEN_COMMA); break;

        case ':': 
            add_token(scanner, TOKEN_COLON); break;
            
        case ' ':
        case '\r':
        case '\t':
            break;

        case '\n':
            scanner->line++;
            break;

        /* Comments go until the end of the line */
        case '#':
            while (peek(scanner) != '\n' && !is_eof(scanner)) {
                advance(scanner);
            }
            
            break;

        case '"': scan_string(scanner);
            break;

        case '@': scan_keyword(scanner);
            break;

        /* A '-' is only valid as the sign of a negative number */
        case '-':
            if (is_digit(peek(scanner))) {
                scan_number_or_time(scanner);
            }
            
            else {
                error(scanner, "Unexpected character '-'.");
            }

            break;

        default:
            if (is_digit(character)) {
                scan_number_or_time(scanner);
            }
            
            else if (is_alpha(character)) {
                scan_identifier(scanner);
            }
            
            else {
                error(scanner, "Unexpected character '%c'.", character);
            }
            
            break;
    }
}

bool scan_tokens(const char* source, TokenList* tokens) {
    Scanner scanner = {
        .source = source,
        .tokens = tokens,
        .start = 0,
        .current = 0,
        .line = 1,
        .had_error = false,
        .out_of_memory = false,
    };

    /* Running out of memory stops scanning, since no later token could be stored */
    while (!is_eof(&scanner) && !scanner.out_of_memory) {
        scanner.start = scanner.current;
        scan_token(&scanner);
    }

    /* Zero-length lexeme since start is moved up to current */
    scanner.start = scanner.current;
    add_token(&scanner, TOKEN_EOF);

    return !scanner.had_error;
}
