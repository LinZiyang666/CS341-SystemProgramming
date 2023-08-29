/**
 * extreme_edge_cases
 * CS 341 - Fall 2023
 */
#include <stdio.h>

#include "camelCaser_ref_utils.h"

int main() {
    // Enter the string you want to test with the reference here.
    char *input = "hello. welcome to cs241";

    // This function prints the reference implementation output on the terminal.
    print_camelCaser(input);

    // Put your expected output for the given input above.
     char *correct[] = {"hello", NULL};
     char *wrong[] = {"hello", "welcomeToCs241", NULL};

    // Compares the expected output you supplied with the reference output.
    printf("check_output test 1: %d\n", check_output(input, correct));
    printf("check_output test 2: %d\n", check_output(input, wrong));

    // Feel free to add more test cases.
    //
    //
    char *input2 = "123Abc 345Efg, 7";
    char *correct2[] = {"123abc345Efg", NULL};

    char *input3 = "123Abc! 345Efg mOM, 7.,!";
    char *correct3[] = {"123abc", "345efgMom", "7", "", "", NULL};

    printf("check_output test 3: %d\n", check_output(input2, correct2));
    printf("check_output test 4: %d\n", check_output(input3, correct3));
    return 0;
}
