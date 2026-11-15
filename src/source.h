/**
 * @file  source.h
 * @brief Allows source.c to be public interface.
 */

#ifndef MSL_SOURCE_H
#define MSL_SOURCE_H

/**
 * @brief   Reads the whole source code file into a null-terminated string.
 * @details The function returns NULL and prints an error to stderr on failure.
 *          The caller owns the returned string and must free() it after scanning.
 */
char* read_source(const char* path);

#endif
