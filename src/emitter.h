/**
 * @file  emitter.h
 * @brief Allows emitter.c to be public interface.
 */

#ifndef MSL_EMITTER_H
#define MSL_EMITTER_H

#include <stdbool.h>
#include <stdio.h>

#include "parser.h"

/**
 * @brief   Writes sequence as JSON to an already open output stream.
 * @details out stays open; the caller still owns it.
 *          The stream is flushed before returning,
 *          so a buffered write error is caught here.
 *          Returns false if any write error occurred.
 *          sequence should come from a successful parse_sequence().
 */
bool write_sequence(FILE* out, const Sequence* sequence);

/**
 * @brief   Writes sequence as JSON to the file at path, replacing any existing contents.
 * @details Every error is printed to stderr.
 *          On a write error the partial file is removed.
 *          Returns false if the file could not be opened, written, or closed.
 *          sequence should come from a successful parse_sequence().
 */
bool write_sequence_file(const char* path, const Sequence* sequence);

#endif
