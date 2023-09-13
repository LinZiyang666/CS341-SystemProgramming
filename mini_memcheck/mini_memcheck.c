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

// Turn off buffering for `stdout` -> done in mini_hacks.c
//  setvbuf(stdout, NULL, _IONBF, 0);

void *mini_malloc(size_t request_size, const char *filename,
                  void *instruction)
{
    // your code here
    if (request_size == 0)
        return NULL; // UB

    meta_data *node = (meta_data *)malloc(sizeof(meta_data) + request_size);

    // If the function fails to allocate the requested block of memory
    if (!node)
        return NULL;

    total_memory_requested += request_size;
    node->filename = filename;
    node->request_size = request_size;
    node->instruction = instruction;

    // Insert to front in LL
    node->next = head;
    head = node;

    return node + 1; // This should be the start of the user's memory, and not the meta_data.
}

void *mini_calloc(size_t num_elements, size_t element_size,
                  const char *filename, void *instruction)
{
    // your code here

    if (num_elements == 0 || element_size == 0)
        return NULL; // UB

    size_t request_size = num_elements * element_size;
    meta_data *node = (meta_data *)malloc(sizeof(meta_data) + request_size);

    // If the function fails to allocate the requested block of memory
    if (!node)
        return NULL;

    total_memory_requested += request_size;
    node->filename = filename;
    node->instruction = instruction;
    node->request_size = request_size;

    // Insert to front in LL
    node->next = head;
    head = node;

    meta_data *memory = node + 1;
    memset(memory, '\0', request_size);

    return memory; // This should be the start of the user's memory, and not the meta_data.
}

void *mini_realloc(void *payload, size_t request_size, const char *filename,
                   void *instruction)
{ // The original memory referenced by payload should not change

    // your code here
    if (payload == NULL && request_size == 0) return NULL; //UB
    else if (payload == NULL)
        return mini_malloc(request_size, filename, instruction);
    else if (request_size == 0) {
        mini_free(payload);
        return NULL;
    }

    meta_data *node = valid_ptr(head, payload);

    if (!node) { // not valid ptr
        invalid_addresses ++;
        return NULL;
    }

    meta_data *memory = node + 1;
    size_t prev_size = node ->request_size;

    if (prev_size < request_size)
        total_memory_requested += (request_size - prev_size);
    else
        total_memory_freed += (prev_size - request_size);
    
    // Modify the references in the LL to the newly allocated node
    if (node == head) {
        node = realloc(node, request_size);
        node ->request_size = request_size;
        head = node;
    }
    else {
        meta_data *before = before_node(head, node);
        node = realloc(node, request_size);
        node -> request_size = request_size;
        before -> next = node;
    }

    return node + 1;    
}

// Return the entire node, the pointer pointing at the start of metadata <=> p1 in visualisation
meta_data *valid_ptr(meta_data *walk, meta_data *ptr)
{
    while (walk)
    {
        meta_data *memory = walk + 1;
        if (memory == ptr)
            return walk;
        else
            walk = walk->next;
    }
    return 0;
}

// Returns the node before `ptr` in the LL; Note that ptr is of type ptr1 from visualisation, that points to start of metadata
meta_data *before_node(meta_data *walk, meta_data *ptr) {

    meta_data *before = walk;
    walk = walk -> next;

    while (walk) {
        if (walk == ptr)
            return before;
        else {
            before = walk;
            walk = walk -> next;
        }

    }

    // should note reach this code
    assert(1); 
    return NULL;
}

void mini_free(void *payload)
{
    // your code here

    if (!payload)
        return;

    meta_data *node = valid_ptr(head, payload);
    if (node)
    { // also remove the metadata node from the list

        // Remove the node from the 'LL' as well
        if (node == head)
        {
            head = head->next;
            total_memory_freed += node->request_size;
            free(node);
        }

        else
        {
            meta_data *before = before_node(head, node);
            before->next = node->next;
            total_memory_freed += node->request_size;
            free(node);
        }

        return;
    }
    else
    {
        invalid_addresses++;
        return;
    }
}
