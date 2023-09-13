/**
 * mini_memcheck
 * CS 341 - Fall 2023
 */
#include "mini_memcheck.h"
#include <stdio.h>
#include <string.h>


meta_data *head;
size_t total_memory_requested;
size_t total_memory_freed;
size_t invalid_addresses;

//Turn off buffering for `stdout` -> done in mini_hacks.c
// setvbuf(stdout, NULL, _IONBF, 0);


void *mini_malloc(size_t request_size, const char *filename,
                  void *instruction) {
    // your code here
    if (request_size == 0) return NULL; //UB

    meta_data *node = (meta_data *) malloc(sizeof(meta_data) + request_size);

    // If the function fails to allocate the requested block of memory
    if (!node)
        return NULL;

    total_memory_requested += request_size; 
    node ->filename = filename;
    node ->request_size = request_size;
    node ->instruction = instruction;
    
    // Insert to front in LL
    node -> next = head;
    head = node; 

    return node + 1; // This should be the start of the user's memory, and not the meta_data.
}

void *mini_calloc(size_t num_elements, size_t element_size,
                  const char *filename, void *instruction) {
    // your code here

    if (num_elements == 0 || element_size == 0) return NULL; //UB

    size_t request_size = num_elements * element_size; 
    meta_data *node = (meta_data *) malloc(sizeof(meta_data) + request_size);

    // If the function fails to allocate the requested block of memory
    if (!node)
        return NULL;
    
    node -> filename = filename;
    node->instruction = instruction;
    node ->request_size = request_size;
    
    // Insert to front in LL
    node -> next = head;
    head = node;

    return node + 1; // This should be the start of the user's memory, and not the meta_data.
}

void *mini_realloc(void *payload, size_t request_size, const char *filename,
                   void *instruction) {
    // your code here
    return NULL;
}

void mini_free(void *payload) {
    // your code here
}
