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

    int **matrix = malloc(1 * sizeof(int*));
    matrix[0] = malloc(1 * sizeof(int));
    matrix[0][0] = 8942;
    double_step(matrix);
    free(matrix[0]);
    matrix[0] = NULL;
    free(matrix); 
    matrix = NULL;

    char *msg = malloc(10);
    strcpy(msg, "VladP");
    for (int i = 5; i < 9; ++i)
      msg[i] = 1;
    msg[9] = '\0';
    free(msg);
    msg = NULL;





    return 0;
}
