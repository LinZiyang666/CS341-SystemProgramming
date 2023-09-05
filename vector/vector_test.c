/**
 * vector
 * CS 341 - Fall 2023
 */
#include "vector.h"
#include <stdio.h>
#include <assert.h>

void print_vector(char *desc, vector *v)
{
    puts("*****************************");
    puts(desc);
    char **walk = (char **)vector_begin(v);
    char **end = (char **)vector_end(v);
    while (walk < end)
    {
        printf("v[%ld]: %s\n", walk - (char **)vector_begin(v), *walk);
        walk++;
    }
    printf("size: %zu\n", vector_size(v));
    printf("capacity: %zu\n", vector_capacity(v));
    printf("vector.empty(): %d\n", vector_empty(v));
    puts("*****************************\n");
}

void run_test(char **arr, size_t n)
{

    vector *v = vector_create(NULL, NULL, NULL); // shallow
    print_vector("Empty vector", v);

    for (size_t i = 0; i < n; ++i)
        vector_push_back(v, arr[i]);
    print_vector("Vector after push_backs", v);

    vector_pop_back(v);
    print_vector("Vector after one pop_back", v);

    // Test iterators
    puts("*****************************");
    printf("Begin of vector: %s\n", (char *)*vector_begin(v));
    printf("End of vector: %s\n", (char *)*vector_end(v));
    puts("*****************************\n");

    // Test: size & capacity, resize & reserve
    puts("*****************************");
    printf("Size of vector: %zu\n", vector_size(v));
    printf("Capacity of vector: %zu\n\n", vector_capacity(v));
    printf("Let's resize the vector w/: n = 12\n");
    vector_resize(v, 12);
    printf("Size of vector: %zu\n", vector_size(v));         // expected 12
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
    // at
    puts("*****************************");
    size_t p = 2;
    char *str = (char *)*vector_at(v, p);
    printf("I expect vector_at[%ld] to be: 3. It is %s. Hooray!\n", p, str);
    puts("*****************************\n");

    // set
    puts("*****************************");
    vector_set(v, p, "Changed");
    printf("I expect vector_at[%ld] to be: Changed. It is %s. Hooray!\n", p, (char *)*vector_at(v, p));
    puts("*****************************\n");

    // get
    puts("*****************************");
    char *obtained = (char *)vector_get(v, 1);
    printf("I expect vector_at[%ld] to be: 2. It is %s. Hooray!\n", (size_t)1, obtained);
    puts("*****************************\n");

    // front
    puts("*****************************");
    obtained = (char *)*vector_front(v);
    printf("I expect vector_front to be: 100. It is %s. Hooray!\n", obtained);
    puts("*****************************\n");

    // back
    puts("*****************************");
    puts("Let's first resize the vector back to 3 elements");
    vector_resize(v, 3);
    printf("vector_size(v): %ld\n", vector_size(v));
    obtained = (char *)*vector_back(v);
    printf("I expect vector_back to be: Changed. It is %s. Hooray!\n", obtained);
    puts("*****************************\n");

    // Test: insert & erase
    //insert
    print_vector("Before insertion", v);
    vector_insert(v, 1, "inserted");
    print_vector("After insertion", v);
    
    //erase
    print_vector("Before erase", v);
    vector_erase(v, 2);
    print_vector("After erase v[2] = 2", v);

    // Test: clear
    puts("*****************************");
    vector_clear(v);
    printf("I expect v.size() == 0, and I obtain: %ld. Hooray!\n", vector_size(v));
    puts("*****************************\n");
}

int **allocate_array_intptrs(size_t n) {
    puts("*****************************");
    printf("Print int array: for n = %ld\n", n);
    int **arr = malloc(sizeof(int *) * n);
    for (size_t i = 0; i < n; ++i) {
        arr[i] = malloc(sizeof(int));
        *arr[i] = i;
        printf("%d ", *arr[i]);
    }
    puts("\n*****************************\n");
    return arr;
}

int main(/* int argc, char *argv[] */)
{
    // Write your test cases here

    // T1 -> shallow copy of char* vector
    run_test((char *[]){"100", "2", "3", "4"}, 4);

    // T2 -> deep copy of int* vector
    size_t n = 4;
    int **arr = allocate_array_intptrs(n);
    // Deallocate
    for (size_t i = 0; i < n; ++i) {
        free(arr[i]);
        arr[i] = NULL;
    }
    free(arr);
    arr = NULL;
    assert(arr == NULL);

    return 0;
}
