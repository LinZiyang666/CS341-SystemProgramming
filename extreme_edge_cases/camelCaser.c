/**
 * extreme_edge_cases
 * CS 341 - Fall 2023
 */
#include "camelCaser.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// You may want to consider using the #define directive for this, especially if
// you're using this in multiple files
//void logg(char *message) {
//#ifdef DEBUG
// fprintf(stderr, "%s\n", message);
// #endif
// }

char **split_str(const char *input_str) {
  char **splitted = NULL;
  size_t input_len =
      strlen(input_str); // If input is NOT NULL, then it is a NUL-terminated
                         // array of characters (a standard C string).
  size_t splitted_len = 0;
  size_t i = 0;

  while (i < input_len) {
    size_t j = i; 
    for (; j < input_len; ++j) // for-loop starts from j = i
      if (ispunct(input_str[j])) {

        size_t nbytes = j - i;
        char *str = malloc(nbytes + 1);
        const char *substr_to_copy = input_str + i;
        strncpy(str, substr_to_copy, nbytes);
        str[nbytes] = '\0'; // remove punctuation !

        splitted_len++;
        splitted = realloc(splitted, splitted_len * sizeof(char *));
        splitted[splitted_len - 1] = str;

        break;
      }
    i = j + 1;
  }

  splitted_len++;
  splitted = realloc(splitted, splitted_len * sizeof(char *));
  splitted[splitted_len - 1] = NULL;
  return splitted;
}

void camel_case_word(char *word, int first_word) {
  char *walk = word;
  int first_letter = 0; // first_letter == 1 -> we have passed over the first
                        // letter of the word
  while (*walk && *walk != ' ') {
    if (isalpha(*walk)) {
      if (!first_letter) {
        first_letter = 1;
        if (first_word)
          *walk = tolower(*walk);
        else
          *walk = toupper(*walk);
      } else {
        *walk = tolower(*walk);
      }

      walk ++;
    }
  }
}

void camel_case_str(char *str) {
  int first_word = 0;
  size_t str_len = strlen(str);
  size_t i = 0;
  while (i < str_len) {
    size_t j = i;
    for ( ; j < str_len; ++j) { // for-loop starts from j = i
      if (!isspace(str[j]) && !first_word)
        first_word = 1; // mark first word, as it has special rules to camelCase

      else if (isspace(str[j]) &&
               first_word) { // we have to camel case the word!
        camel_case_word(str + i, first_word);
        // `remove_space_str`
        break;
      }

      else if (j == str_len - 1) // word has ended, but there is no space after
                                 // it => camel case it
      {
        camel_case_word(str + i, first_word);
        break;
      }
    }
    i = j + 1;
  }
}

void remove_spaces_str(char *str) {
  size_t str_len = strlen(str);
  char *new_str = malloc(str_len + 1);
  size_t new_str_len = 0;

  for (size_t i = 0; i < str_len; ++i) {
    if (!isspace(str[i]))
      new_str[new_str_len++] = str[i];
  }
  new_str[str_len] = '\0';
  strncpy(str, new_str, new_str_len);
  free(new_str);
}

char **camel_caser(const char *input_str) {
  if (input_str == NULL)
    return NULL;

  char **splitted =
      split_str(input_str); // TODO: Make sure to deallocate after returning

  char **walk = splitted;
  if (*walk) {
    camel_case_str(*walk); // A input sentence, input_s, is defined as any
                           // MAXIMAL substring of the input string that ends
                           // with a punctuation mark. (!) First camel case the
                           // string, then remove spaces from it
    remove_spaces_str(*walk); // i.e: input = " hello world ", now *walk == "
                              // hello World ".; we should get rid of spaces
    walk++;
  }
  return splitted;
}

void destroy(char **result) { // Check picture in docs:
                              // https://cs341.cs.illinois.edu/assignments/extreme_edge_cases
                              // ; deallocate all char * and then the initial
                              // char** that pointe to all of them
  if (result == NULL)
    return;

  char **walk = result;
  while (*walk) { // for each C-string
    free(*walk);   // free the C-string
    walk++;       // go to next C-string
  }

  free(result); // free char**
}
