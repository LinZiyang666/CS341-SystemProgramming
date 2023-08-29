/**
 * extreme_edge_cases
 * CS 341 - Fall 2023
 */
#include "camelCaser.h"
#include <stdlib.h>

char **camel_caser(const char *input_str) {
    // TODO: Implement me!
    //
    if (input_str == NULL) return NULL;
    return NULL;
}

void destroy(char **result) {
    if (result == NULL) return; 

    char **walk = result;
    while (*walk) { // for each C-string
        free(walk); // free it
        walk ++; // go to next C-string
    }

    free(result); // free char** 
    return;
}
