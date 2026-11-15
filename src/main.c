/**
 * @file  main.c
 * @brief Entry point for the MSL CLI tool.
 * TODO: Update this header.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scanner.h"
#include "source.h"

/* Printable names for each TokenType, indexed by the enum value */
static const char* const TOKEN_TYPE_NAMES[] = {
    [TOKEN_LEFT_PAREN]    = "LEFT_PAREN",
    [TOKEN_RIGHT_PAREN]   = "RIGHT_PAREN",
    [TOKEN_LEFT_BRACKET]  = "LEFT_BRACKET",
    [TOKEN_RIGHT_BRACKET] = "RIGHT_BRACKET",
    [TOKEN_LEFT_BRACE]    = "LEFT_BRACE",
    [TOKEN_RIGHT_BRACE]   = "RIGHT_BRACE",
    [TOKEN_COMMA]         = "COMMA",
    [TOKEN_COLON]         = "COLON",
    [TOKEN_METADATA]      = "METADATA",
    [TOKEN_EPOCH]         = "EPOCH",
    [TOKEN_ABSOLUTE]      = "ABSOLUTE",
    [TOKEN_RELATIVE]      = "RELATIVE",
    [TOKEN_TRUE]          = "TRUE",
    [TOKEN_FALSE]         = "FALSE",
    [TOKEN_IDENTIFIER]    = "IDENTIFIER",
    [TOKEN_STRING]        = "STRING",
    [TOKEN_NUMBER]        = "NUMBER",
    [TOKEN_TIME]          = "TIME",
    [TOKEN_EOF]           = "EOF",
};

_Static_assert(sizeof(TOKEN_TYPE_NAMES) / sizeof(TOKEN_TYPE_NAMES[0]) == TOKEN_TYPE_COUNT, "TOKEN_TYPE_NAMES must have one entry per TokenType");

/* TODO: Refine usage and general CLI tool design */
static void print_usage(void) {
    fprintf(stderr, "usage: msl <input.msl> [-o <output.msl.json>] [--tokens]\n");
}

/* For scanner tokenlist output */
static void print_tokens(const TokenList* list) {
    printf("%-6s %-14s %s\n", "LINE", "TYPE", "LEXEME");

    for (size_t i = 0; i < list->count; i++) {
        const Token* token = &list->tokens[i];
        printf("%-6d %-14s %.*s\n", token->line, TOKEN_TYPE_NAMES[token->type], (int)token->length, token->start);
    }
}

int main(int argc, char* argv[]) {
    const char* input_path = NULL;
    const char* output_path = NULL;
    bool dump_tokens = false;

    /* Accept input path, -o <output>, and --tokens in any order (like gcc) */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc || output_path != NULL) {
                print_usage();
                return 64;
            }

            output_path = argv[++i];
        }

        else if (strcmp(argv[i], "--tokens") == 0) {
            dump_tokens = true;
        }

        else if (input_path == NULL) {
            input_path = argv[i];
        }

        else {
            print_usage();
            return 64;
        }
    }

    if (input_path == NULL) {
        print_usage();
        return 64;
    }

    char* source = read_source(input_path);
    if (source == NULL) {
        return 74;
    }

    TokenList tokens = { .tokens = NULL, .count = 0, .capacity = 0 };
    bool scanned = scan_tokens(source, &tokens);

    /* Printed even after errors, since the scanner keeps going past them */
    if (dump_tokens) {
        print_tokens(&tokens);
    }

    /* TODO: Parse tokens and write JSON to output_path */
    (void)output_path;

    free(tokens.tokens);
    free(source);
    return scanned ? 0 : 65;
}
