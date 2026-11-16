/**
 * @file  main.c
 * @brief Entry point for the MSL CLI tool.
 *        Calls:
 *            msl.exe <input.msl> --tokens            Dump scanned tokens, then report any parse errors
 *            msl.exe <input.msl> -o <output.json>    Compile to JSON
 *            msl.exe --help                          Show help (also shown when called with no arguments)
 *       
 *        Exactly one option is allowed per call.
 *        The input path and the option may come in either order.
 *       
 *        Exit codes (BSD sysexits.h values):
 *            0  Success, or help shown
 *            64 Usage error (bad arguments)
 *            65 Data error (scan or parse errors in the input)
 *            74 I/O error (input could not be read, or output could not be written)
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "emitter.h"
#include "parser.h"
#include "scanner.h"
#include "source.h"

#define EXIT_USAGE    64
#define EXIT_DATA_ERR 65
#define EXIT_IO_ERR   74

/* Printable names for each TokenType, indexed by the enum value */
static const char* const TOKEN_TYPE_NAMES[] = {
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

typedef enum {
    MODE_NONE,
    MODE_HELP,
    MODE_TOKENS,
    MODE_OUTPUT,
} Mode;

typedef struct {
    Mode mode;
    const char* input_path;
    const char* output_path;
} Options;

/* Help goes to stdout when asked for, and to stderr after a usage error */
static void print_help(FILE* out) {
    fputs(
        "Usage: msl.exe <input.msl> <option>\n"
        "Options:\n"
        "    --help                  Show this help information.\n"
        "    --tokens                Print the tokens scanned from <input.msl>.\n"
        "    -o <output.msl.json>    Compile <input.msl> to JSON at <output.msl.json>.\n",
        out
    );
}

/* Prints a usage error followed by the help, and returns the usage exit code */
static int usage_error(const char* message, const char* argument) {
    if (argument != NULL) {
        fprintf(stderr, "error: %s \"%s\".\n\n", message, argument);
    }

    else {
        fprintf(stderr, "error: %s.\n\n", message);
    }

    print_help(stderr);
    return EXIT_USAGE;
}

/* For scanner tokenlist output */
static void print_tokens(const TokenList* list) {
    printf("%-6s %-14s %s\n", "LINE", "TYPE", "LEXEME");

    for (size_t i = 0; i < list->count; i++) {
        const Token* token = &list->tokens[i];
        printf("%-6d %-14s %.*s\n", token->line, TOKEN_TYPE_NAMES[token->type], (int)token->length, token->start);
    }
}

/* Sets the one option for this call; a second option is a usage error */
static bool set_mode(Options* options, Mode mode) {
    if (options->mode != MODE_NONE) {
        return false;
    }

    options->mode = mode;
    return true;
}

/**
 * @brief   Reads argv into options.
 * @details Returns 0 on success, or the exit code after printing a usage error.
 */
static int parse_arguments(int argc, char* argv[], Options* options) {
    for (int i = 1; i < argc; i++) {
        const char* argument = argv[i];

        /* An empty argument, e.g. msl.exe "" -o out.json */
        if (argument[0] == '\0') {
            return usage_error("empty argument", NULL);
        }

        Mode mode = MODE_NONE;
        if (strcmp(argument, "--help") == 0) {
            mode = MODE_HELP;
        }

        else if (strcmp(argument, "--tokens") == 0) {
            mode = MODE_TOKENS;
        }

        else if (strcmp(argument, "-o") == 0) {
            mode = MODE_OUTPUT;
        }

        if (mode != MODE_NONE) {
            if (!set_mode(options, mode)) {
                return usage_error("only one option is allowed per call; unexpected", argument);
            }

            if (mode != MODE_OUTPUT) {
                continue;
            }

            /* -o needs a path after it that is not empty and not another option */
            if (i + 1 >= argc || argv[i + 1][0] == '\0' || argv[i + 1][0] == '-') {
                return usage_error("expected an output path after", "-o");
            }

            options->output_path = argv[++i];
        }

        /* Any other dash argument, including a lone "-" (stdin is not supported) */
        else if (argument[0] == '-') {
            return usage_error("unknown option", argument);
        }

        else if (options->input_path == NULL) {
            options->input_path = argument;
        }

        else {
            return usage_error("unexpected extra argument", argument);
        }
    }

    if (options->mode == MODE_HELP) {
        if (options->input_path != NULL) {
            return usage_error("--help takes no other arguments; unexpected", options->input_path);
        }

        return 0;
    }

    if (options->input_path == NULL) {
        return usage_error("missing input file", NULL);
    }

    if (options->mode == MODE_NONE) {
        return usage_error("missing option (--tokens or -o)", NULL);
    }

    /* Exact match only; a differently spelled path to the same file is not caught */
    if (options->mode == MODE_OUTPUT && strcmp(options->input_path, options->output_path) == 0) {
        return usage_error("output path would overwrite the input file", options->output_path);
    }

    return 0;
}

int main(int argc, char* argv[]) {
    /* No arguments: show the help, as --help does */
    if (argc < 2) {
        print_help(stdout);
        return EXIT_SUCCESS;
    }

    Options options = { .mode = MODE_NONE, .input_path = NULL, .output_path = NULL };
    int usage = parse_arguments(argc, argv, &options);
    if (usage != 0) {
        return usage;
    }

    if (options.mode == MODE_HELP) {
        print_help(stdout);
        return EXIT_SUCCESS;
    }

    char* source = read_source(options.input_path);
    if (source == NULL) {
        return EXIT_IO_ERR;
    }

    TokenList tokens = { .tokens = NULL, .count = 0, .capacity = 0 };
    bool scanned = scan_tokens(source, &tokens);

    /* Printed even after errors, since the scanner keeps going past them */
    if (options.mode == MODE_TOKENS) {
        print_tokens(&tokens);
    }

    /* Skipped after scanner errors, since missing tokens would only cause cascading parse errors */
    Sequence sequence = { 0 };
    bool parsed = scanned && parse_sequence(&tokens, &sequence);

    int status = parsed ? EXIT_SUCCESS : EXIT_DATA_ERR;

    /* Only written after a clean parse, since opening the output replaces its contents */
    if (parsed && options.mode == MODE_OUTPUT && !write_sequence_file(options.output_path, &sequence)) {
        status = EXIT_IO_ERR;
    }

    free_sequence(&sequence);
    free(tokens.tokens);
    free(source);
    return status;
}
