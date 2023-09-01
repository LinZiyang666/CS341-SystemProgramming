/**
 * perilous_pointers
 * CS 341 - Fall 2023
 */
#include "part2-functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * (Edit this function to print out the "Illinois" lines in
 * part2-functions.c in order.)
 */
int main() {

  first_step(81);

  int i132 = 132;
  second_step(&i132);

  int **matrix = malloc(1 * sizeof(int *));
  matrix[0] = malloc(1 * sizeof(int));
  matrix[0][0] = 8942;
  double_step(matrix);
  free(matrix[0]);
  matrix[0] = NULL;
  free(matrix);
  matrix = NULL;

  char *msg = malloc(9);
  strcpy(msg, "VladP");
  *(int *)(msg + 5) = 15;
  strange_step(msg);
  free(msg);
  msg = NULL;

  msg = malloc(4);
  msg[3] = 0;
  empty_step(msg);
  free(msg);
  msg = NULL;

  msg = malloc(4);
  msg[3] = 'u'; 
  two_step(msg, msg); 
  free(msg);
  msg = NULL;

  msg = "abcde"; 
  three_step(msg, msg + 2, msg + 4); // "abcde", "cde" and "e" 
  
  msg = "\0aiq"; 
  step_step_step(msg, msg, msg);

  msg = malloc(4);
  * (int *)(msg) = 5;
  int b = 5;
  it_may_be_odd(msg, b);
  free(msg);
  msg = NULL;

  msg = strdup("Course,CS241"); // after checking `strtok` man page BUGS section: strtok cannot be used on `const literals`
  tok_step(msg);
  free(msg);
  msg = NULL;

  msg = malloc(4); 
  msg[0] = 1;
  msg[1] = 2; 
  msg[2] = msg[3] = '\0';
  the_end(msg, msg);
  free(msg); 
  msg = NULL;

  return 0;
}
