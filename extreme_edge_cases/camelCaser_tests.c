/**
 * extreme_edge_cases
 * CS 341 - Fall 2023
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "camelCaser.h"
#include "camelCaser_tests.h"

/// @brief Compute the number of C-string  in the table of camel cased C-strings
/// @param split table of camel cased C-strings
/// @return `size_t` representing number of camel cased C-strings
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

/* Fails to compile on Autograder. as `split_str` is not known
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
*/ 

/// @brief Auxiliar method that is called after we end running a test (i.e.: after `run_test` method call ends)
/// @param result obtained output by calling `camelCaser` method implementation of CS341 staff that we have to test.
/// @param exit_code  1, if successful, 0 otherwise
/// @param destroy  function pointer of CS341 staff implementation of `destroy` method that we have to test
/// @return exit code
int exit_run_test(char **result, int exit_code, void (*destroy)(char **)) {
  destroy(result);
  return exit_code;
}

/// @brief Helper method to reduce test code; Runs `camelCaser` on `input` and stores the result in `char **obtained_output`; then compares `obtained_output` to `expected_output` for containing the same content
/// @param camelCaser function pointer of CS341 staff implementation of `camel_caser` method that we have to test 
/// @param destroy  function pointer of CS341 staff implementation of `destroy` method that we have to test
/// @param input the C-string that I input for this specific test case scenario
/// @param expected_output the table of camel-cased C-strings that is expected.
/// @return 1, if successful, 0 else.
int run_test(char **(*camelCaser)(const char *), void (*destroy)(char **),
             char *input, char **expected_output) {

  char **obtained_output = camelCaser(input);

  size_t expected_output_len = get_size(expected_output);
  size_t obtained_output_len = get_size(obtained_output);

  if (expected_output_len != obtained_output_len) {
    return exit_run_test(obtained_output, 0, destroy);
  }

  char **walk_expected = expected_output;
  char **walk_obtained = obtained_output;

  while (*walk_expected && *walk_obtained) {
    if (strcmp(*walk_expected, *walk_obtained) != 0)
      return exit_run_test(obtained_output, 0, destroy);
    walk_expected++;
    walk_obtained++;
  }

  return exit_run_test(obtained_output, 1, destroy);
}

int test_camelCaser(char **(*camelCaser)(const char *),
                    void (*destroy)(char **)) {

  char *in =
      "The Heisenbug is an incredible creature. Facenovel servers get their "
      "power from its indeterminism. Code smell can be ignored with INCREDIBLE "
      "use of air freshener. God objects are the new religion.";
  char *out[] = {"theHeisenbugIsAnIncredibleCreature",
                 "facenovelServersGetTheirPowerFromItsIndeterminism",
                 "codeSmellCanBeIgnoredWithIncredibleUseOfAirFreshener",
                 "godObjectsAreTheNewReligion", NULL};
  if (run_test(camelCaser, destroy, in, out) == 0)
    return 0;

  if (run_test(camelCaser, destroy, "123Abc! 345Efg mOM, 7.,!",// multibehavior test
               (char *[]){"123abc", "345efgMom", "7", "", "", NULL}) == 0)
    return 0;

  if (run_test(camelCaser, destroy, "1\tab3! 345Efg mOM, 7.,!", // \t test
               (char *[]){"1Ab3", "345efgMom", "7", "", "", NULL}) == 0)
    return 0;

  if (run_test(camelCaser, destroy, ",.!?", // Full punctuation test
               (char *[]){"", "", "", "", NULL}) == 0)
    return 0;

  if (run_test(camelCaser, destroy, " ?. ", // Punctuation & spaces only
               (char *[]){"", "", NULL}) == 0)
    return 0;

  if (run_test(camelCaser, destroy, "1\t2a\ab3c!?", // Combination tabs, digits & letters
               (char *[]){"12A\ab3c", "", NULL}) == 0) // NOTE: !isspace('\a') == false;  \a (alert) Produces an audible or visible alert without changing the active position.
    return 0;

  // String already terminated test (print nothing)
  if (run_test(camelCaser, destroy, "abc \0stop.", 
               (char *[]){NULL}) == 0) 
    return 0;

  // String already terminated test (print nothing)
  if (run_test(camelCaser, destroy, " \0stop.", 
               (char *[]){NULL}) == 0) 
    return 0;
  
  // All caps test
  if (run_test(camelCaser, destroy, "HI THERE I AM VLAD!", 
               (char *[]){"hiThereIAmVlad", NULL}) == 0) 
    return 0;
  
  // All lower test
  if (run_test(camelCaser, destroy, "hi there i am vlad?", 
               (char *[]){"hiThereIAmVlad", NULL}) == 0)  // same output as above
    return 0;
  // return test_split_str(); -> to test if I split correctly
  return 1;
}
