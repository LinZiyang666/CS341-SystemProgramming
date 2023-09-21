/**
 * teaching_threads
 * CS 341 - Fall 2023
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "reduce.h"
#include "reducers.h"

/* You might need a struct for each task ... */
struct worklist_t{
    int *left_value; // left_boundary offsetted by `list`
    int *right_value; // right_boundary offsetted by `list`
    int acc;  // accumulator
    reducer callback; 
};
typedef struct worklist_t worklist_t;

/* You should create a start routine for your threads. */
void* solve_worker (void *arg) {

    worklist_t *worker = (worklist_t *) arg; 

    for (int *addr = worker-> left_value; addr < worker -> right_value; ++addr) // add(add(add(0, 1), 2), 3) = 6. -> FOLD LEFT, even if associative reduce functions => does not matter fold-left OR fold-right
        worker->acc = worker->callback(worker->acc, *addr); // Dereference the value: list[addr_boundary] -> see initialisation of `left_vaue` and `right_value` attributes

    return NULL;
}

int par_reduce(int *list, size_t list_len, reducer reduce_func, int base_case,
               size_t num_threads) {
    /* Your implementation goes here */

    if (list_len < num_threads) // Edge case: make sure you run each el. in a single thread
        num_threads = list_len; //  make sure to always spawn the least number of threads?
    
    worklist_t *workers = calloc(num_threads, sizeof(worklist_t));
        for (size_t thread_id = 0; thread_id < num_threads; ++thread_id) { // Set up workers boundaries 
        // IDEA: keep track of the boundary offsetted by the `list` mem. addr (start of list), so that we can access list[i] inside thread_funtion
        workers[thread_id].left_value = list + list_len * thread_id / num_threads; 
        workers[thread_id].right_value = list + list_len * (thread_id + 1) / num_threads;
        // Split evenly as shown during the Lab by TA
        workers[thread_id].acc = base_case;
        workers[thread_id].callback = reduce_func;
    }


    pthread_t threads[num_threads]; // On stack
    for (size_t thread_id = 0; thread_id < num_threads; ++thread_id) {
            pthread_create(threads + thread_id, NULL, solve_worker, workers + thread_id);
            if (thread_id + 1 == num_threads) // last thread created -> join all threads
             {
                for (size_t i = 0; i < num_threads; ++i)
                pthread_join(threads[i], NULL);
             }
    }

    // Combine all workers' work
    int par_reduced = base_case;
    for (size_t thread_id = 0; thread_id < num_threads; ++thread_id) { 
        par_reduced = reduce_func(par_reduced, workers[thread_id].acc); // fold-left
    }


    free(workers);
    return par_reduced;

    return 0;
}
