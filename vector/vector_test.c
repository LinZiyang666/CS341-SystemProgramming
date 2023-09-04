/**
 * vector
 * CS 341 - Fall 2023
 */
#include "vector.h"
#include <stdio.h>


void print_int_vector(char* desc, vector *v){
    puts("*****************************");
    puts(desc);
    char **walk = (char **)vector_begin(v);
    char **end = (char **)vector_end(v);
    while (walk < end) {
        printf("v[%ld]: %s\n", walk - (char **)vector_begin(v), *walk);
        walk++;
    }
    printf("size: %zu\n", vector_size(v));
    printf("capacity: %zu\n", vector_capacity(v));
    printf("vector.empty(): %d\n", vector_empty(v));
    puts("*****************************\n");
}

void run_test(char **arr, size_t n) {

    vector *v = vector_create(NULL, NULL, NULL); // shallow
    print_int_vector("Empty vector", v);

    for (size_t i = 0; i < n; ++i)
        vector_push_back(v, arr[i]);
    print_int_vector("Vector after push_backs", v);

    vector_pop_back(v);
    print_int_vector("Vector after one pop_back", v);

    // Test iterators
    puts("*****************************");
    printf("Begin of vector: %s\n", (char *) *vector_begin(v));
    printf("End of vector: %s\n", (char *) *vector_end(v));
    puts("*****************************\n");

    // Test: size & capacity, resize & reserve
    puts("*****************************");
    printf("Size of vector: %zu\n", vector_size(v));
    printf("Capacity of vector: %zu\n\n", vector_capacity(v));
    printf("Let's resize the vector w/: n = 12\n");
    vector_resize(v, 12); 
    printf("Size of vector: %zu\n", vector_size(v)); // expected 12
    printf("Capacity of vector: %zu\n", vector_capacity(v)); // expected 16
    puts("*****************************\n\n");

    puts("*****************************");
    printf("Let's reserve vector: n = 50\n");
    vector_reserve(v, 50); // expected: capacity = 64, the first power of 2 >= 50
    printf("Size of vector: %zu\n", vector_size(v)); 
    printf("Capacity of vector: %zu\n", vector_capacity(v)); 
    puts("*****************************\n\n");

    puts("*****************************");
    printf("Let's reserve vector: n = 2; This should have no effect! \n");
    vector_reserve(v, 2);
    printf("Size of vector: %zu\n", vector_size(v)); 
    printf("Capacity of vector: %zu\n", vector_capacity(v)); 
    puts("*****************************\n");

    // Element access tests

}

int main(/* int argc, char *argv[] */) {
    // Write your test cases here

    // T1 -> shallow copy of char* vector
    run_test((char* []){"100", "2", "3", "4"}, 4);

    return 0;
}
