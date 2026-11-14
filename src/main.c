/**
 * @file  main.h
 * @brief 
 * TODO: Update this header.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "source.h"

/* TODO: Refine usage and general CLI tool design */
static void print_usage(void) {
    fprintf(stderr, "usage: msl <input.msl> [-o <output.msl.json>]\n");
}

int main(int argc, char* argv[]) {
    const char* input_path = NULL;
    const char* output_path = NULL;

    /* Accept input path and -o <output> in any order (like gcc) */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc || output_path != NULL) {
                print_usage();
                return 64;
            }

            output_path = argv[++i];
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

    /* TEMPORARY: echoes source until scanner and JSON output exist */
    printf("input:  %s\n", input_path);
    printf("output: %s\n", output_path != NULL ? output_path : "(not set)");
    printf("bytes:  %zu\n", strlen(source));
    printf("----------------------------------------\n");
    printf("%s\n", source);

    free(source);
    return 0;
}
