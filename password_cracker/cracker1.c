/**
 * password_cracker
 * CS 341 - Fall 2023
 */
#include "cracker1.h"
#include "format.h"
#include "utils.h"
#include <crypt.h>
#include <pthread.h>
#include "includes/queue.h"
#include <stdio.h>
#include <string.h>


#define UNBOUNDED (-1)
#define TRUE 1
#define FALSE 0
#define SENTINEL_Q NULL

// maude xxEe0WApyDMcg a......
struct task {
    char *username; 
    char *hashed_pwd; 
    char *known_pwd; // contains `...` at the end
};

typedef struct task task_t;

static pthread_mutex_t numRecoveredMtx= PTHREAD_MUTEX_INITIALIZER; // successful password cracks
static pthread_mutex_t numFailedMtx = PTHREAD_MUTEX_INITIALIZER; // unsuccessful (failed) password cracks

static int numRecovered = 0;
static int numFailed = 0;

queue *Q = NULL;


void parse_line(char *lineptr, char **username, char **hashed_pwd, char **known_pwd) {
    *username = strtok(lineptr, " ");
    *hashed_pwd = strtok(lineptr, " ");
    *known_pwd = strtok(lineptr, " ");
}

task_t *create_task(char *username, char *hashed_pwd, char *known_pwd) {
        task_t *tsk = malloc(sizeof(task_t));
        tsk->username = strdup(username);
        tsk->hashed_pwd = strdup(hashed_pwd);
        tsk->known_pwd = strdup(known_pwd);
        return tsk;
}



int start(size_t thread_count) {
    // TODO your code here, make sure to use thread_count!
    // Remember to ONLY crack passwords in other threads

    Q = queue_create(UNBOUNDED);
    char *lineptr = NULL;
    size_t n = 0;
    size_t nbytes = 0;

    while ((nbytes = getline(&lineptr, n, stdin))) { // read from STDIN 
        
        if (nbytes == -1) break; 
        if (nbytes > 0 && lineptr[nbytes - 1] == '\n') // remove '\n' appended by getline
            lineptr[nbytes - 1] == '\0';

        char *username = NULL;
        char *hashed_pwd = NULL;
        char *known_pwd = NULL;
        parse_line(lineptr, &username, &hashed_pwd, &known_pwd);

        task_t *tsk = create_task(username, hashed_pwd, known_pwd);

        queue_push(Q, tsk);
    }
    free(lineptr);

    queue_push(Q, SENTINEL_Q); // the analogue of the null byte)

    // Create `thread_count` threads
    pthread_t tids[thread_count];
    for (int i = 0; i < thread_count; ++i) {
        pthread_create(tids + i, NULL, solve_task, (void *)(i + 1)); // tids are 1-indexed !
        if (i + 1 == thread_count) // last thread has been created
            for (int j = 0; j < thread_count; ++j) //  Every thread must join with the main thread!
                pthread_join(tids[i], NULL);
    }


    v1_print_summary(numRecovered, numFailed);
    queue_destroy(Q);

    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}
