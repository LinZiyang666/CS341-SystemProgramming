/**
 * extreme_edge_cases
 * CS 341 - Fall 2023
 */
#include "camelCaser.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// @brief Method that splits the `input_str` into a table of MAXIMAL substrings (of the input string) that ends with a punctuation mark.
/// @param input_str the input string that we want to camel_case
/// @return `char **` table of C-strings with the property described before
char **split_str(const char *input_str)
{
  char **splitted = NULL;
  size_t input_len =
      strlen(input_str); // If input is NOT NULL, then we are guaranteed it is a NULL-terminated
                         // array of characters (a standard C string).
  size_t splitted_len = 0;
  size_t i = 0;

  while (i < input_len)
  {
    size_t j = i;
    for (; j < input_len; ++j) // inner for-loop starts from j = i
      if (ispunct(input_str[j]))
      {

        // copy s[i..j] substring
        size_t nbytes = j - i;
        char *str = malloc(nbytes + 1);
        const char *substr_to_copy = input_str + i;
        strncpy(str, substr_to_copy, nbytes);
        str[nbytes] = '\0'; // (!) remove the punctuation mark

        splitted_len++;
        splitted = realloc(splitted, splitted_len * sizeof(char *)); // allocate an additional `char *` mem. space for the new string that we will insert
        splitted[splitted_len - 1] = str; // insert the new sting

        break;
      }
    i = j + 1;
  }

  // `camel_caser` returns an array of output_s for every input_s in the input string, terminated by a NULL pointer.
  splitted_len++;
  splitted = realloc(splitted, splitted_len * sizeof(char *));
  splitted[splitted_len - 1] = NULL;
  return splitted;
}

/// @brief  Take a given word C-string as input, and depending whether or not it is the first word we parse, transform (camel case) it accordingly.
/// @param word a substring of a C-string that needs to be camel cased, according to different rules depending on `first_word` value
/// @param first_word = 1, if `char *word` is the first word we parse, or 0 otherwise.
void camel_case_word(char *word, int first_word)
{
  char *walk = word;
  int first_letter = 0; // if (first_letter == 1) -> we have passed over the first
                        // letter of the word / we have encountered the first letter already
  while (*walk)
  {
    if (isalpha(*walk))
    {
      if (!first_letter)
      {
        first_letter = 1;
        if (first_word)
          *walk = tolower(*walk);
        else
          *walk = toupper(*walk);
      }
      else
      {
        *walk = tolower(*walk);
      }
    }
    walk++;
  }
}

/// @brief camel_case the C-string received as input, while not modifying the spaces. We will have a separate function that removes the spaces, after camel casing.
/// @param str MAXIMAL substring of the input that is to be camel cased
void camel_case_str(char *str)
{
  int first_word_seen = 0; // first_word_seen = 1 iff. we already found the first word.
  size_t str_len = strlen(str);
  size_t i = 0;
  while (i < str_len)
  {
    size_t j = i;
    for (; j < str_len; ++j)
    { // for-loop starts from j = i
      if (!isspace(str[j]) && !first_word_seen)
        first_word_seen = 1; // mark first word, as we handle it as an edge-case when camel casing

      else if (isspace(str[j]) &&
               first_word_seen)
      { // camel case s[i... j - 1] substring

        // Mark the space as NULL byte to know where to stop in `camel_case_word`
        str[j] = '\0';                   
        camel_case_word(str + i, i == 0); // if (i == 0) -> still parsing first word
        str[j] = ' ';                     // restore space, as we will remove it in `remove_whitespaces_str`
        break;
      }

      else if (j == str_len - 1) // word has ended, but there is no space after
                                 // it => we still have to camel case s[i... j - 1] substring
      {
        camel_case_word(str + i, i == 0); // if (i == 0) -> still parsing first word
        break;
      }
    }
    i = j + 1;
  }
}

/// @brief Remove all whitespaces from `char *str`. 
/// Use an auxiliar `char *new_str` to construct the C-string w/o  whitespaces, and then `strcpy` it to `char *str`
//  NOTE: Function has side-effects (modifies `char *str` arg. received as input)
/// @param str the C-string of which whitespaces we want to remove.
void remove_whitespaces_str(char *str)
{
  size_t str_len = strlen(str);
  char *new_str = malloc(str_len + 1); 
  size_t new_str_len = 0;

  for (size_t i = 0; i < str_len; ++i)
  {
    if (!isspace(str[i]))
      new_str[new_str_len++] = str[i];
  }
  new_str[new_str_len] = '\0';
  strncpy(str, new_str, new_str_len);
  str[new_str_len] = '\0';
  free(new_str);
}

/// @brief Camel Case the `char * input_str`, remove all spaces and separate all sentences in a table of strings. For more info check assignment's description
/// @param input_str C-string received as argument, can also be `NULL`
/// @return `char**` representing the table of strings, each string being camel-cased.
char **camel_caser(const char *input_str)
{
  if (input_str == NULL)
    return NULL;

  char **splitted =
      split_str(input_str);

  char **walk = splitted;
  while (*walk)
  { // traverse the table of strings
    // for each C-string `walk`

    // camel_case it
    camel_case_str(*walk); // A input sentence, `input_s`, is defined as any
                           // MAXIMAL substring of the input string that ends
                           // with a punctuation mark.
                           // (!) First camel case the
                           // string, then remove spaces from it
    // and then remove its spaces
    remove_whitespaces_str(*walk); // i.e: input = " hello world ", now *walk == "
                              // hello World ".; we should remove these spaces
    walk++;
  }
  return splitted;
}

void destroy(char **result)
{ // Check picture in docs:
  // https://cs341.cs.illinois.edu/assignments/extreme_edge_cases
  // ; deallocate each C-string and then the initial
  // `char**` that points to all of them

  if (result == NULL) // if already NULL, nothing to be destroyed
    return;

  char **walk = result;
  while (*walk)  // parse the table of camel_cased strigns
  {              // for each C-string
    free(*walk); // free the C-string
    walk++;      // go to next C-string
  }

  free(result); // free `char**`

  /*
  The key is to always free memory in the same manner as you malloc'ed it.
  So, if you malloc a 2-dimensional array and then malloc a series of 1-dimensional arrays,
  you need to do the reverse when freeing it

  Source: https://stackoverflow.com/questions/2483869/how-to-properly-free-a-char-table-in-c
  */
}
