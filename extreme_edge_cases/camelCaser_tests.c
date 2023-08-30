/**
 * extreme_edge_cases
 * CS 341 - Fall 2023
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "camelCaser.h"
#include "camelCaser_tests.h"

size_t get_size(char **split) {
  if (split == NULL)
    return 0;

  size_t size = 0;
  char **walk = split;
  while (*walk) {
    size++;
    walk++;
  }
  return size;
}

int test_split_str() {
  const char *input_str =
      "The Heisenbug is an incredible creature. Facenovel servers get their "
      "power from its indeterminism. Code smell can be ignored with INCREDIBLE "
      "use of air freshener. God objects are the new religion.";

  char *expected_split[] = {
      "The Heisenbug is an incredible creature",
      " Facenovel servers get their power from its indeterminism",
      " Code smell can be ignored with INCREDIBLE use of air freshener",
      " God objects are the new religion", NULL};

  char **obtained_split = split_str(input_str);

  size_t obtained_split_len = get_size(obtained_split);
  size_t expected_split_len = get_size(expected_split);

  if (obtained_split_len != expected_split_len)
    return 0; 

  char **walk = expected_split;
  while (*walk) {
    if (!strcmp(*walk, *obtained_split)) { // equal
      walk++;
      obtained_split++;
    } else
      return 0;
  }

  return *obtained_split == NULL;
}

int test_camelCaser(char **(*camelCaser)(const char *),
                    void (*destroy)(char **)) {
  // TODO: Implement me!


  // return test_split_str(); -> to test if I split correctly
  return test_split_str();
}
