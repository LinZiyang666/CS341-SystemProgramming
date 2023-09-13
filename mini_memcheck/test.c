/**
 * mini_memcheck
 * CS 341 - Fall 2023
 */
#include <stdio.h>
#include <stdlib.h>

void test1()
{
    void *p1 = malloc(30);
    void *p2 = malloc(40);
    void *p3 = malloc(50);
    free(p3);
}

void test_noleaks()
{
    void *p1 = malloc(30);
    void *p2 = malloc(40);
    void *p3 = malloc(50);
    free(p1);
    free(p2);
    free(p3);
}

void test_3leaks()
{
    void *p1 = malloc(30);
    void *p2 = malloc(40);
    void *p3 = malloc(50);
}

void test_double_free()
{
    void *p1 = malloc(30);
    free(p1);
    free(p1); // Oops
}

void test_free_invalid_address()
{
    void *p1 = malloc(30);
    free(p1 - 1); // Oops
}

void test_realloc_invalid_address()
{
    void *p1 = malloc(30);
    realloc(p1 - 1, 60); // Oops
}

void test_calloc_no_memleaks()
{
    size_t num_elements = 2;
    size_t size_elements = 15;
    void *p2 = calloc(num_elements, size_elements);
    free(p2);
}

void test_variety_of_issues()
{

    char *p1 = malloc(10);
    *(p1 + 10) = 'a'; // Write out of bounds
    // Don't free p1

    char *p2 = malloc(5);
    free(p2);
    free(p2); // free twice
}

void test_realloc_no_memleaks() {
    char *p = malloc(10);
    p = realloc(p, 100);
    free(p);
    return ;
}

void realloc_elaborate() {
    void *p1 = malloc(30);
    void *p3 = malloc(50);
    p3 = realloc(p3, 10);
    
    free(p1);
    // free(p3); -> should leak 10 BYTES
}

int main(int argc, char *argv[])
{
    // Your tests here using malloc and free

    /* Exploratory */
    // test1();
    // test_noleaks();
    // test_3leaks();
    // test_double_free();
    /* test */ realloc_elaborate();
    // + test echo via CLI

    /* Lab4 tests*/
    // test_free_invalid_address();
    // test_realloc_invalid_address();

    // test_calloc_no_memleaks();

    // test_variety_of_issues();

    // test_realloc_no_memleaks();



    return 0;
}