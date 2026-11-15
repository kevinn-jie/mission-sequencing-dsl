/**
 * @file  parser.c
 * @brief Parses a list of tokens into a sequence.
 */

/*
 * MSL grammar
 *
 * Rules:
 *     A sequence is every metadata header, then every step, then the end of the file.
 *         sequence   -> metadata* step* EOF ;
 *
 *     A metadata header is a key and its value, both strings.
 *         metadata   -> "@METADATA" STRING STRING ;
 *         e.g.          @METADATA "author" "kevin_jie"
 *
 *     A step is a time frame, a timestamp, a stem, and any number of arguments.
 *     A step ends where the next time frame keyword begins.
 *         step       -> time_frame TIME stem argument* ;
 *         time_frame -> "@EPOCH" | "@ABSOLUTE" | "@RELATIVE" ;
 *         stem       -> IDENTIFIER ;
 *         argument   -> value ;
 *         e.g.          @RELATIVE 00:00:20.000 COMMAND3 "argument" [123, true]
 *
 *     A value is a string, a number, a boolean, an array, or an object.
 *         value      -> STRING | NUMBER | boolean | array | object ;
 *         boolean    -> "true" | "false" ;
 *
 *     An array is a comma-separated list of values inside brackets.
 *         array      -> "[" elements? "]" ;
 *         elements   -> value ( "," value )* ;
 *         e.g.          ["argument", 123, true]
 *
 *     An object is a comma-separated list of entries inside braces.
 *     An entry is an identifier key, a colon, and a value.
 *         object     -> "{" entries? "}" ;
 *         entries    -> entry ( "," entry )* ;
 *         entry      -> IDENTIFIER ":" value ;
 *         e.g.          {key1: "argument", key2: 123}
 *
 * Checks made outside the grammar:
 *     The scanner emits absolute and relative timestamps as the same TIME token
 *     and checks their format only, so the parser checks that:
 *       - TIME is absolute (YYYY-DDDTHH:MM:SS.sss) after "@EPOCH" and
 *         "@ABSOLUTE".
 *       - TIME is relative (HH:MM:SS.sss) after "@RELATIVE".
 *       - Each TIME field is in range (day of year, hour, minute, second).
 *
 * Unconfirmed choices in the rules above:
 *     Ordering:
 *       - A metadata header after the first step is an error.
 *       - "@EPOCH" is optional, may repeat, and may appear at any step.
 *     Values:
 *       - A metadata value must be a string.
 *       - Arrays and objects may be empty: [] and {}
 *       - Arrays and objects may nest: [[1, 2], {key1: [3]}]
 *       - A trailing comma is an error: [1, 2,]
 *     Unused tokens:
 *       - "(" and ")" are scanned, but no rule uses them, so they are always an error.
 * 
 * Choice confirmation:
 *     Ordering:
 *       - Metadata headers can appear anywhere, they simply translate to the metadata section.
 *       - "@EPOCH" is required because it denotes the beginning of a sequence.
 *     Values:
 *       - Metadata values can be booleans, strings, numbers, arrays, and objects, but the key must be a string.
 *         Identifiers do no work with metadata.
 *       - Arrays and objects can be empty: [] and {}
 *       - Arrays and objects can nest: [[1, 2], {key1: [3]}]
 *       - A trailing comma is an error: [1, 2,]
 *     Unused tokens:
 *       - They are not needed, they can be removes and unintegrated.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "scanner.h"

typedef struct {
    const TokenList* tokens;
    size_t current;
    bool had_error;
    bool panic_mode; /* Suppresses later errors until synchronize() */
} Parser;

static const Token* peek(const Parser* parser) {
    return &parser->tokens->tokens[parser->current];
}

static const Token* previous(const Parser* parser) {
    return &parser->tokens->tokens[parser->current - 1];
}

static bool is_at_end(const Parser* parser) {
    return peek(parser)->type == TOKEN_EOF;
}

static const Token* advance(Parser* parser) {
    if (!is_at_end(parser)) {
        parser->current++;
    }

    return previous(parser);
}

static bool check(const Parser* parser, TokenType type) {
    if (is_at_end(parser)) {
        return false;
    }

    return peek(parser)->type == type;
}

static bool match(Parser* parser, TokenType type) {
    if (!check(parser, type)) {
        return false;
    }

    advance(parser);
    return true;
}

static void error_at(Parser* parser, const Token* token, const char* message) {
    if (parser->panic_mode) {
        return;
    }

    parser->panic_mode = true;
    parser->had_error = true;

    fprintf(stderr, "error: line %d", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    }

    else {
        fprintf(stderr, " at '%.*s'", (int)token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
}

static bool consume(Parser* parser, TokenType type, const char* message) {
    if (check(parser, type)) {
        advance(parser);
        return true;
    }

    error_at(parser, peek(parser), message);
    return false;
}

/**
 * @brief Discards tokens until the next metadata header or step begins.
 *
 * @details Leaves panic mode so later errors are reported again. "@METADATA",
 *          "@EPOCH", "@ABSOLUTE", and "@RELATIVE" are MSL's statement
 *          boundaries, so the token at one of them is kept for the next rule.
 */
static void synchronize(Parser* parser) {
    parser->panic_mode = false;

    while (!is_at_end(parser)) {
        switch (peek(parser)->type) {
            case TOKEN_METADATA:
            case TOKEN_EPOCH:
            case TOKEN_ABSOLUTE:
            case TOKEN_RELATIVE:
                return;

            default:
                advance(parser);
                break;
        }
    }
}

