/**
 * extreme_edge_cases
 * CS 341 - Fall 2023
 */
#include "camelCaser.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// You may want to consider using the #define directive for this, especially if
// you're using this in multiple files
void logg(char *message) {
#ifdef DEBUG
  fprintf(stderr, "%s\n", message);
#endif
}

char **split_str(const char *input_str) {
  char **splitted = NULL;
  size_t input_len =
      strlen(input_str); // If input is NOT NULL, then it is a NUL-terminated
                         // array of characters (a standard C string).
  size_t splitted_len = 0;

  for (size_t i = 0; i < input_len; ++i) {
    for (size_t j = i; j < input_len; ++j)
      if (ispunct(input_str[j])) {

        size_t nbytes = j - i;
        char *str = malloc(nbytes + 1);
        const char *substr_to_copy = input_str + i;
        strncpy(str, substr_to_copy, nbytes);
        str[nbytes + 1] = '\0'; // remove punctuation ! 


        splitted_len++;
        splitted = realloc(splitted, splitted_len * sizeof(char *));
        splitted[splitted_len - 1] = str;

        i = j + 1;
        break;
      }
  }

  return splitted;
}

char **camel_caser(const char *input_str) {
  if (input_str == NULL)
    return NULL;

  char **splitted = split_str(input_str);

  char **walk = splitted;
  if (*walk) {
    // split_words(*walk); // A input sentence, input_s, is defined as any MAXIMAL
                       // substring of the input string that ends with a
                       // punctuation mark.
    // remove_punctuation(
        // walk); // The punctuation from input_s is not added to output_s.
    walk++;
  }
  return splitted;
}

void destroy(char **result) {
  if (result == NULL)
    return;

  char **walk = result;
  while (*walk) { // for each C-string
    free(walk);   // free it
    walk++;       // go to next C-string
  }

  free(result); // free char**
  return;
}
