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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "scanner.h"

/* Bounds recursion so deeply nested arrays and objects cannot overflow the stack */
#define MAX_NESTING_DEPTH 256

/* Lexeme lengths of the two TIME formats the scanner accepts */
#define ABSOLUTE_TIME_LENGTH 21 /* YYYY-DDDTHH:MM:SS.sss */
#define RELATIVE_TIME_LENGTH 12 /* HH:MM:SS.sss          */

typedef struct {
    const TokenList* tokens;
    size_t current;
    bool had_error;
    bool panic_mode;    /* Suppresses later errors until synchronize() */
    bool out_of_memory; /* Stops parsing entirely                      */
    bool seen_step;     /* Whether a time frame keyword has been parsed */
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

/**
 * @brief   Reports an error at `token` without entering panic mode.
 * @details Used for checks made outside the grammar,
 *          where the tokens are still well-formed and parsing can continue normally.
 */
static void report(Parser* parser, const Token* token, const char* message) {
    if (parser->panic_mode) {
        return;
    }

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

/* Reports a syntax error and enters panic mode until synchronize() */
static void error_at(Parser* parser, const Token* token, const char* message) {
    report(parser, token, message);
    parser->panic_mode = true;
}

static void out_of_memory(Parser* parser) {
    fprintf(stderr, "error: not enough memory to store the sequence.\n");
    parser->had_error = true;
    parser->panic_mode = true;
    parser->out_of_memory = true;
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
 * @brief   Discards tokens until the next metadata header or step begins.
 * @details Leaves panic mode so later errors are reported again.
 *          "@METADATA", "@EPOCH", "@ABSOLUTE", and "@RELATIVE" are MSL's statement boundaries,
 *          so the token at one of them is kept for the next rule.
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

/**
 * @brief   Makes room for one more item in a list.
 * @details Returns the list's items, moved if they had to grow,
 *          or NULL if the list cannot grow. On failure the existing items are left intact.
 */
static void* reserve(Parser* parser, void* items, size_t* capacity, size_t count, size_t item_size) {
    if (count < *capacity) {
        return items;
    }

    size_t new_capacity = *capacity < 8 ? 8 : *capacity * 2;
    if (new_capacity > SIZE_MAX / item_size) {
        out_of_memory(parser);
        return NULL;
    }

    void* grown = realloc(items, new_capacity * item_size);
    if (grown == NULL) {
        out_of_memory(parser);
        return NULL;
    }

    *capacity = new_capacity;
    return grown;
}

static bool push_value(Parser* parser, ValueList* list, Value item) {
    Value* items = reserve(parser, list->items, &list->capacity, list->count, sizeof(Value));
    if (items == NULL) {
        return false;
    }

    list->items = items;
    list->items[list->count++] = item;
    return true;
}

static bool push_entry(Parser* parser, EntryList* list, Entry item) {
    Entry* items = reserve(parser, list->items, &list->capacity, list->count, sizeof(Entry));
    if (items == NULL) {
        return false;
    }

    list->items = items;
    list->items[list->count++] = item;
    return true;
}

static bool push_step(Parser* parser, StepList* list, Step item) {
    Step* items = reserve(parser, list->items, &list->capacity, list->count, sizeof(Step));
    if (items == NULL) {
        return false;
    }

    list->items = items;
    list->items[list->count++] = item;
    return true;
}

static void free_value(Value* value);

static void free_value_list(ValueList* list) {
    for (size_t i = 0; i < list->count; i++) {
        free_value(&list->items[i]);
    }

    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static void free_entry_list(EntryList* list) {
    for (size_t i = 0; i < list->count; i++) {
        free_value(&list->items[i].value);
    }

    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static void free_value(Value* value) {
    if (value->type == VALUE_ARRAY) {
        free_value_list(&value->as.array);
    }

    else if (value->type == VALUE_OBJECT) {
        free_entry_list(&value->as.object);
    }
}

static bool value(Parser* parser, Value* out, int depth);

/**
 * @brief   array -> "[" elements? "]" ; elements -> value ( "," value )* ;
 * @details The opening "[" has already been consumed. On failure everything parsed so far is freed.
 */
static bool array(Parser* parser, Value* out, int depth) {
    out->type = VALUE_ARRAY;
    out->token = *previous(parser);
    out->as.array = (ValueList){ .items = NULL, .count = 0, .capacity = 0 };

    if (!check(parser, TOKEN_RIGHT_BRACKET)) {
        do {
            if (check(parser, TOKEN_RIGHT_BRACKET)) {
                error_at(parser, previous(parser), "Trailing comma is not allowed in an array.");
                goto fail;
            }

            Value element;
            if (!value(parser, &element, depth + 1)) {
                goto fail;
            }

            if (!push_value(parser, &out->as.array, element)) {
                free_value(&element);
                goto fail;
            }
        } while (match(parser, TOKEN_COMMA));
    }

    if (!consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' or ',' after array element.")) {
        goto fail;
    }

    return true;

fail:
    free_value(out);
    return false;
}

/**
 * @brief   entry -> IDENTIFIER ":" value ;
 * @details On failure nothing is left allocated.
 */
static bool entry(Parser* parser, Entry* out, int depth) {
    if (!consume(parser, TOKEN_IDENTIFIER, "Expected an identifier key in object.")) {
        return false;
    }

    out->key = *previous(parser);

    if (!consume(parser, TOKEN_COLON, "Expected ':' after object key.")) {
        return false;
    }

    return value(parser, &out->value, depth + 1);
}

/**
 * @brief   object -> "{" entries? "}" ; entries -> entry ( "," entry )* ;
 * @details The opening "{" has already been consumed. On failure everything parsed so far is freed.
 */
static bool object(Parser* parser, Value* out, int depth) {
    out->type = VALUE_OBJECT;
    out->token = *previous(parser);
    out->as.object = (EntryList){ .items = NULL, .count = 0, .capacity = 0 };

    if (!check(parser, TOKEN_RIGHT_BRACE)) {
        do {
            if (check(parser, TOKEN_RIGHT_BRACE)) {
                error_at(parser, previous(parser), "Trailing comma is not allowed in an object.");
                goto fail;
            }

            Entry item;
            if (!entry(parser, &item, depth)) {
                goto fail;
            }

            if (!push_entry(parser, &out->as.object, item)) {
                free_value(&item.value);
                goto fail;
            }
        } while (match(parser, TOKEN_COMMA));
    }

    if (!consume(parser, TOKEN_RIGHT_BRACE, "Expected '}' or ',' after object entry.")) {
        goto fail;
    }

    return true;

fail:
    free_value(out);
    return false;
}

/**
 * @brief   value -> STRING | NUMBER | boolean | array | object ;
 *          boolean -> "true" | "false" ;
 * @details On failure nothing is left allocated.
 */
static bool value(Parser* parser, Value* out, int depth) {
    if (depth > MAX_NESTING_DEPTH) {
        error_at(parser, peek(parser), "Arrays and objects are nested too deeply.");
        return false;
    }

    switch (peek(parser)->type) {
        case TOKEN_STRING:
            out->type = VALUE_STRING;
            out->token = *advance(parser);
            return true;

        case TOKEN_NUMBER:
            out->type = VALUE_NUMBER;
            out->token = *advance(parser);
            return true;

        case TOKEN_TRUE:
        case TOKEN_FALSE:
            out->type = VALUE_BOOLEAN;
            out->token = *advance(parser);
            out->as.boolean = out->token.type == TOKEN_TRUE;
            return true;

        case TOKEN_LEFT_BRACKET:
            advance(parser);
            return array(parser, out, depth);

        case TOKEN_LEFT_BRACE:
            advance(parser);
            return object(parser, out, depth);

        default:
            error_at(parser, peek(parser), "Expected a value (string, number, boolean, array, or object).");
            return false;
    }
}

/* Reads count decimal digits; the scanner has already checked they are digits */
static int read_digits(const char* text, int count) {
    int result = 0;

    for (int i = 0; i < count; i++) {
        result = result * 10 + (text[i] - '0');
    }

    return result;
}

static int days_in_year(int year) {
    bool is_leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    return is_leap ? 366 : 365;
}

/**
 * @brief   Checks the TIME after a time frame keyword (made outside the grammar).
 * @details "@EPOCH" and "@ABSOLUTE" need YYYY-DDDTHH:MM:SS.sss,
 *          "@RELATIVE" needs HH:MM:SS.sss, and every field must be in range.
 *          Relative hours are not bounded beyond their two digits.
 */
static void check_time(Parser* parser, const Token* time_frame, const Token* time) {
    bool is_absolute = time->length == ABSOLUTE_TIME_LENGTH;
    bool needs_absolute = time_frame->type != TOKEN_RELATIVE;

    if (needs_absolute && !is_absolute) {
        report(parser, time, "Expected an absolute timestamp (YYYY-DDDTHH:MM:SS.sss) after '@EPOCH' or '@ABSOLUTE'.");
    }

    else if (!needs_absolute && is_absolute) {
        report(parser, time, "Expected a relative timestamp (HH:MM:SS.sss) after '@RELATIVE'.");
    }

    /* Points at HH in either format */
    const char* clock = time->start;

    if (is_absolute) {
        int year = read_digits(time->start, 4);
        int day = read_digits(time->start + 5, 3);
        clock = time->start + 9;

        if (day < 1 || day > days_in_year(year)) {
            report(parser, time, "Day of year is out of range (001-365, or 366 in a leap year).");
        }

        if (read_digits(clock, 2) > 23) {
            report(parser, time, "Hour is out of range (00-23).");
        }
    }

    if (read_digits(clock + 3, 2) > 59) {
        report(parser, time, "Minute is out of range (00-59).");
    }

    if (read_digits(clock + 6, 2) > 59) {
        report(parser, time, "Second is out of range (00-59).");
    }
}

static bool is_time_frame(TokenType type) {
    return type == TOKEN_EPOCH || type == TOKEN_ABSOLUTE || type == TOKEN_RELATIVE;
}

/* Metadata headers and time frame keywords begin a statement and end a step */
static bool is_statement_start(TokenType type) {
    return type == TOKEN_METADATA || is_time_frame(type);
}

/**
 * @brief   metadata -> "@METADATA" STRING value ;
 * @details The "@METADATA" keyword has already been consumed.
 *          Per the confirmed choices, the key must be a string and the value may be any value.
 */
static void metadata(Parser* parser, Sequence* sequence) {
    if (!consume(parser, TOKEN_STRING, "Expected a string key after '@METADATA'.")) {
        return;
    }

    Entry header = { .key = *previous(parser) };

    if (!value(parser, &header.value, 0)) {
        return;
    }

    if (!push_entry(parser, &sequence->metadata, header)) {
        free_value(&header.value);
    }
}

/**
 * @brief   step -> time_frame TIME stem argument* ;
 *          stem -> IDENTIFIER ; argument -> value ;
 * @details The time frame keyword has already been consumed.
 *          Arguments run until the next statement keyword or the end of the file.
 *          Per the confirmed choices, "@EPOCH" begins the sequence,
 *          so it must be the first step and cannot appear again.
 */
static void step(Parser* parser, Sequence* sequence) {
    Step result = { .time_frame = *previous(parser) };
    result.args = (ValueList){ .items = NULL, .count = 0, .capacity = 0 };

    if (!parser->seen_step && result.time_frame.type != TOKEN_EPOCH) {
        report(parser, &result.time_frame, "Expected the sequence to begin with an '@EPOCH' step.");
    }

    else if (parser->seen_step && result.time_frame.type == TOKEN_EPOCH) {
        report(parser, &result.time_frame, "'@EPOCH' may only appear once, as the first step.");
    }

    parser->seen_step = true;

    if (!consume(parser, TOKEN_TIME, "Expected a timestamp after the time frame keyword.")) {
        return;
    }

    result.time = *previous(parser);
    check_time(parser, &result.time_frame, &result.time);

    if (!consume(parser, TOKEN_IDENTIFIER, "Expected a command stem after the timestamp.")) {
        return;
    }

    result.stem = *previous(parser);

    while (!is_at_end(parser) && !is_statement_start(peek(parser)->type)) {
        Value argument;
        if (!value(parser, &argument, 0)) {
            free_value_list(&result.args);
            return;
        }

        if (!push_value(parser, &result.args, argument)) {
            free_value(&argument);
            free_value_list(&result.args);
            return;
        }
    }

    if (!push_step(parser, &sequence->steps, result)) {
        free_value_list(&result.args);
    }
}

/* Parses one metadata header or one step */
static void statement(Parser* parser, Sequence* sequence) {
    if (match(parser, TOKEN_METADATA)) {
        metadata(parser, sequence);
    }

    else if (is_time_frame(peek(parser)->type)) {
        advance(parser);
        step(parser, sequence);
    }

    else {
        error_at(parser, peek(parser), "Expected '@METADATA', '@EPOCH', '@ABSOLUTE', or '@RELATIVE'.");
    }
}

/**
 * @brief   sequence -> ( metadata | step )* EOF ;
 * @details Per the confirmed choices, metadata headers may appear anywhere,
 *          so they are collected into the metadata section wherever they occur.
 */
bool parse_sequence(const TokenList* tokens, Sequence* sequence) {
    sequence->metadata = (EntryList){ .items = NULL, .count = 0, .capacity = 0 };
    sequence->steps = (StepList){ .items = NULL, .count = 0, .capacity = 0 };

    if (tokens->count == 0 || tokens->tokens[tokens->count - 1].type != TOKEN_EOF) {
        fprintf(stderr, "error: token list does not end with an end-of-file token.\n");
        return false;
    }

    Parser parser = {
        .tokens = tokens,
        .current = 0,
        .had_error = false,
        .panic_mode = false,
        .out_of_memory = false,
        .seen_step = false,
    };

    while (!is_at_end(&parser) && !parser.out_of_memory) {
        statement(&parser, sequence);

        if (parser.panic_mode && !parser.out_of_memory) {
            synchronize(&parser);
        }
    }

    if (!parser.seen_step && !parser.out_of_memory) {
        report(&parser, peek(&parser), "Expected an '@EPOCH' step to begin the sequence.");
    }

    return !parser.had_error;
}

void free_sequence(Sequence* sequence) {
    free_entry_list(&sequence->metadata);

    for (size_t i = 0; i < sequence->steps.count; i++) {
        free_value_list(&sequence->steps.items[i].args);
    }

    free(sequence->steps.items);
    sequence->steps.items = NULL;
    sequence->steps.count = 0;
    sequence->steps.capacity = 0;
}
