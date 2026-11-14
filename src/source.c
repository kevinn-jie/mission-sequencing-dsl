#include <stdio.h>
#include <stdlib.h>

#include "source.h"

char* read_source(const char* path) {
    /* Binary mode, so `ftell()` returns a byte count */
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "error: could not open file \"%s\".\n", path);
        return NULL;
    }

    if (fseek(file, 0L, SEEK_END) != 0) {
        fprintf(stderr, "error: could not seek in file \"%s\".\n", path);
        fclose(file);
        return NULL;
    }

    long file_size = ftell(file);
    if (file_size < 0) {
        fprintf(stderr, "error: could not get size of file \"%s\".\n", path);
        fclose(file);
        return NULL;
    }

    rewind(file);

    /* One extra byte for the null terminator */
    char* buffer = malloc((size_t)file_size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "error: not enough memory to read \"%s\".\n", path);
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, sizeof(char), (size_t)file_size, file);
    if (bytes_read < (size_t)file_size) {
        fprintf(stderr, "error: could not read file \"%s\".\n", path);
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[bytes_read] = '\0';
    fclose(file);
    return buffer;
}
