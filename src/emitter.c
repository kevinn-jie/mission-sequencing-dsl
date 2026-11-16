/**
 * @file  emitter.c
 * @brief Writes a parsed sequence as JSON.
 */

/*
 * Output shape (see examples/design_choices.md, "General Output JSON Structure"):
 *
 *     {
 *       "metadata": {
 *         "<key>": <value>,
 *         ...
 *       },
 *       "steps": [
 *         {
 *           "stem": "<stem>",
 *           "time": {
 *             "type": "<time type>",
 *             "tag": "<timestamp>"
 *           },
 *           "args": [
 *             <value>,
 *             ...
 *           ]
 *         },
 *         ...
 *       ]
 *     }
 *
 * Unconfirmed choices:
 *   - Time types: "@EPOCH" maps to "EPOCH" for now. The reference
 *     "EPOCH_RELATIVE" type uses an offset tag, but the MSL "@EPOCH"
 *     tag is an absolute timestamp.
 *   - Raw control bytes in strings (for example, a tab) are escaped here
 *     instead of being rejected by the scanner.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "emitter.h"
#include "parser.h"
#include "token.h"

/* Spaces per nesting level, matching the reference .seq.json files */
#define INDENT_WIDTH 2

static void write_value(FILE* out, const Value* value, int depth);

static void write_indent(FILE* out, int depth) {
    for (int i = 0; i < depth * INDENT_WIDTH; i++) {
        fputc(' ', out);
    }
}

/* Lexemes are not null-terminated; precision bounds the bytes written */
static void write_lexeme(FILE* out, const Token* token) {
    fprintf(out, "%.*s", (int)token->length, token->start);
}

static void write_quoted(FILE* out, const Token* token) {
    fputc('"', out);
    write_lexeme(out, token);
    fputc('"', out);
}

/**
 * @brief   Writes a STRING token, quotes and escapes included.
 * @details Existing escapes were checked by the scanner and are copied as-is.
 *          JSON does not allow raw control bytes (below 0x20) in a string,
 *          so any that got through the scanner are escaped here.
 */
static void write_string(FILE* out, const Token* token) {
    for (size_t i = 0; i < token->length; i++) {
        unsigned char c = (unsigned char)token->start[i];

        if (c >= 0x20) {
            fputc(c, out);
            continue;
        }

        switch (c) {
            case '\b': fputs("\\b", out);
                break;
                
            case '\f': fputs("\\f", out);
                break;
                
            case '\n': fputs("\\n", out);
                break;
                
            case '\r': fputs("\\r", out);
                break;
                
            case '\t': fputs("\\t", out);
                break;
                
            default:   fprintf(out, "\\u%04x", (unsigned int)c); break;
        }
    }
}

/* Metadata keys are STRING tokens; object keys are IDENTIFIER tokens */
static void write_key(FILE* out, const Token* key) {
    if (key->type == TOKEN_STRING) {
        write_string(out, key);
    }

    else {
        write_quoted(out, key);
    }
}

static void write_value_list(FILE* out, const ValueList* list, int depth) {
    if (list->count == 0) {
        fputs("[]", out);
        return;
    }

    fputs("[\n", out);

    for (size_t i = 0; i < list->count; i++) {
        /* Separator before every element but the first, so there is no trailing comma */
        if (i > 0) {
            fputs(",\n", out);
        }

        write_indent(out, depth + 1);
        write_value(out, &list->items[i], depth + 1);
    }

    fputc('\n', out);
    write_indent(out, depth);
    fputc(']', out);
}

static void write_entry_list(FILE* out, const EntryList* list, int depth) {
    if (list->count == 0) {
        fputs("{}", out);
        return;
    }

    fputs("{\n", out);

    for (size_t i = 0; i < list->count; i++) {
        if (i > 0) {
            fputs(",\n", out);
        }

        write_indent(out, depth + 1);
        write_key(out, &list->items[i].key);
        fputs(": ", out);
        write_value(out, &list->items[i].value, depth + 1);
    }

    fputc('\n', out);
    write_indent(out, depth);
    fputc('}', out);
}

static void write_value(FILE* out, const Value* value, int depth) {
    switch (value->type) {
        case VALUE_STRING:
            write_string(out, &value->token);
            break;

        case VALUE_NUMBER:
        case VALUE_BOOLEAN:
            write_lexeme(out, &value->token);
            break;

        case VALUE_ARRAY:
            write_value_list(out, &value->as.array, depth);
            break;

        case VALUE_OBJECT:
            write_entry_list(out, &value->as.object, depth);
            break;
    }
}

/* Maps a time frame keyword to its JSON time type */
static const char* time_type(TokenType time_frame) {
    switch (time_frame) {
        case TOKEN_EPOCH:
            /* Unconfirmed; see the comment at the top of this file */
            return "EPOCH";

        case TOKEN_ABSOLUTE:
            return "ABSOLUTE";

        case TOKEN_RELATIVE:
            return "COMMAND_RELATIVE";

        default:
            /* Unreachable after a successful parse */
            return "UNKNOWN";
    }
}

static void write_step(FILE* out, const Step* step, int depth) {
    fputs("{\n", out);

    write_indent(out, depth + 1);
    fputs("\"stem\": ", out);
    write_quoted(out, &step->stem);
    fputs(",\n", out);

    write_indent(out, depth + 1);
    fputs("\"time\": {\n", out);

    write_indent(out, depth + 2);
    fprintf(out, "\"type\": \"%s\",\n", time_type(step->time_frame.type));

    write_indent(out, depth + 2);
    fputs("\"tag\": ", out);
    write_quoted(out, &step->time);
    fputc('\n', out);

    write_indent(out, depth + 1);
    fputs("},\n", out);

    write_indent(out, depth + 1);
    fputs("\"args\": ", out);
    write_value_list(out, &step->args, depth + 1);
    fputc('\n', out);

    write_indent(out, depth);
    fputc('}', out);
}

static void write_step_list(FILE* out, const StepList* list, int depth) {
    if (list->count == 0) {
        fputs("[]", out);
        return;
    }

    fputs("[\n", out);

    for (size_t i = 0; i < list->count; i++) {
        if (i > 0) {
            fputs(",\n", out);
        }

        write_indent(out, depth + 1);
        write_step(out, &list->items[i], depth + 1);
    }

    fputc('\n', out);
    write_indent(out, depth);
    fputc(']', out);
}

bool write_sequence(FILE* out, const Sequence* sequence) {
    fputs("{\n", out);

    write_indent(out, 1);
    fputs("\"metadata\": ", out);
    write_entry_list(out, &sequence->metadata, 1);
    fputs(",\n", out);

    write_indent(out, 1);
    fputs("\"steps\": ", out);
    write_step_list(out, &sequence->steps, 1);
    fputc('\n', out);

    fputs("}\n", out);

    /* fflush() reports buffered write errors; ferror() reports any earlier ones */
    bool flushed = fflush(out) == 0;
    return flushed && !ferror(out);
}

bool write_sequence_file(const char* path, const Sequence* sequence) {
    /* "w" replaces existing contents; "b" keeps "\n" from becoming "\r\n" on Windows */
    FILE* file = fopen(path, "wb");
    if (file == NULL) {
        fprintf(stderr, "error: could not open file \"%s\" for writing.\n", path);
        return false;
    }

    bool written = write_sequence(file, sequence);

    /* fclose() flushes buffered data, so it can fail too */
    bool closed = fclose(file) == 0;

    if (!written || !closed) {
        fprintf(stderr, "error: could not write file \"%s\".\n", path);

        /* Removed after fclose(), since Windows cannot delete an open file */
        remove(path);
        return false;
    }

    return true;
}
