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
    char *known_pwd; // contains `...` at the end; NOT HASHED
};

typedef struct task task_t;

static pthread_mutex_t numRecoveredMtx= PTHREAD_MUTEX_INITIALIZER; // successful password cracks
static pthread_mutex_t numFailedMtx = PTHREAD_MUTEX_INITIALIZER; // unsuccessful (failed) password cracks

static int numRecovered = 0;
static int numFailed = 0;

queue *Q = NULL;


void parse_line(char *lineptr, char **username, char **hashed_pwd, char **known_pwd) {
    *username = strtok(lineptr, " ");
    *hashed_pwd = strtok(NULL, " ");
    *known_pwd = strtok(NULL, " ");
}

task_t *create_task(char *username, char *hashed_pwd, char *known_pwd) {
        task_t *tsk = malloc(sizeof(task_t));
        tsk->username = strdup(username);
        tsk->hashed_pwd = strdup(hashed_pwd);
        tsk->known_pwd = strdup(known_pwd);
        return tsk;
}

void task_destroy (task_t *tsk) {
    free(tsk->username);
    free(tsk->hashed_pwd);
    free(tsk->known_pwd);
    free(tsk);
}


bool samePrefix (char *pwd, char *known_pwd) {
    int prefix_size = getPrefixLength(known_pwd); 
    if (strncmp(pwd, known_pwd, prefix_size) == 0) 
        return TRUE;
    else return FALSE;
}

void *solve_task(void *arg) { 

    task_t *tsk = NULL;
    char *hashed = NULL;
    size_t tid = (size_t)arg;

    double thread_start_time = getThreadCPUTime();

    struct crypt_data cdata;
    cdata.initialized = 0;

    while((tsk = (task_t*) queue_pull(Q))) {
        v1_print_thread_start(tid, tsk->username);
        int hashes = 0; 
        int succ = FALSE; 

        char *pwd = strdup(tsk->known_pwd);
        int dotsStart = getPrefixLength(tsk->known_pwd); 
        setStringPosition(pwd + dotsStart, 0); // known_pwd = 'xbc....' => 'xbcaaaa'

        int done = FALSE;
        while (!done) {
            hashed = crypt_r(pwd, "xx", &cdata);
            hashes ++;

            if (strcmp(hashed, tsk->hashed_pwd) == 0) // success
            {
                succ = TRUE;
                double thread_finish_time = getThreadCPUTime() - thread_start_time; 
                v1_print_thread_result(tid, tsk->username, pwd, hashes, thread_finish_time, 0);

                pthread_mutex_lock(&numRecoveredMtx);
                numRecovered ++;
                pthread_mutex_unlock(&numRecoveredMtx);

                done = TRUE;
            } 

            incrementString(pwd); 
            if (!samePrefix(pwd, tsk->known_pwd)) 
                done = TRUE; 
        }

        if (!succ) {
            double thread_finish_time = getThreadCPUTime() - thread_start_time; 

            v1_print_thread_result(tid, tsk->username, NULL, hashes, thread_finish_time, 1);

            pthread_mutex_lock(&numFailedMtx);
            numFailed ++;
            pthread_mutex_unlock(&numFailedMtx);
        }

        free(pwd); 
        if (tsk)
            task_destroy(tsk);        
    }
    
    queue_push(Q, NULL);  // After dequeing: If the next value is the sentinel, push it back to indicate the queue is still empty.
    return NULL;
}

int start(size_t thread_count) {
    // TODO your code here, make sure to use thread_count!
    // Remember to ONLY crack passwords in other threads

    Q = queue_create(UNBOUNDED);
    char *lineptr = NULL;
    size_t n = 0;
    ssize_t nbytes = 0;

    while ((nbytes = getline(&lineptr, &n, stdin))) { // read from STDIN 
        
        if (nbytes == -1) break; 
        if (nbytes > 0 && lineptr[nbytes - 1] == '\n') // remove '\n' appended by getline
            lineptr[nbytes - 1] = '\0';

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
    for (size_t i = 0; i < thread_count; ++i) {
        pthread_create(tids + i, NULL, solve_task, (void *)(i + 1)); // tids are 1-indexed !
        if (i + 1 == thread_count) // last thread has been created
            for (size_t j = 0; j < thread_count; ++j) //  Every thread must join with the main thread!
                pthread_join(tids[j], NULL);
    }


    v1_print_summary(numRecovered, numFailed);
    queue_destroy(Q);

    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}
