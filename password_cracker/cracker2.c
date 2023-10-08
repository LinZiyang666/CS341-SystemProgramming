/**
 * password_cracker
 * CS 341 - Fall 2023
 */
#include "cracker2.h"
#include "format.h"
#include "utils.h"
#include <crypt.h>
#include <pthread.h>
#include "includes/queue.h"
#include <stdio.h>
#include <string.h>

#define MAX_USERNAME 8
#define MAX_PASSWORD_HASH 13
#define TRUE 1
#define FALSE 0

// maude xxEe0WApyDMcg a......
struct task
{
    char *username;
    char *hashed_pwd;
    char *known_pwd; // contains `...` at the end; NOT HASHED
};

typedef struct task task_t;

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_rwlock_t foundLock = PTHREAD_RWLOCK_INITIALIZER;
pthread_barrier_t barrier;
static int total_hashes = 0;
static int THREAD_COUNT = 0; // global var for thread count so that I don't have to pass it to each thread individually
static int found = FALSE;    // TRUE: if a thread found the password, so that the other workers on the same task can stop
task_t *curr_task = NULL;    // All THREAD_COUNT threads work on one task `curr_task` at a time
char *curr_password = NULL;
static int task_done = FALSE;

void parse_line(char *lineptr, char **username, char **hashed_pwd, char **known_pwd)
{
    *username = strtok(lineptr, " ");
    *hashed_pwd = strtok(NULL, " ");
    *known_pwd = strtok(NULL, " ");
}

task_t *create_task(char *username, char *hashed_pwd, char *known_pwd)
{
    task_t *tsk = malloc(sizeof(task_t));
    tsk->username = strdup(username);
    tsk->hashed_pwd = strdup(hashed_pwd);
    tsk->known_pwd = strdup(known_pwd);
    return tsk;
}

void task_destroy(task_t *tsk)
{
    free(tsk->username);
    free(tsk->hashed_pwd);
    free(tsk->known_pwd);
    free(tsk);
}

void allocate_curr_task()
{ // allocate worst-case scenario, so tht we can reuse the same piece of memory for all the tasks we read, and then do just one `free()` at the end of the program
    curr_task = malloc(sizeof(task_t));
    curr_task->username = calloc((MAX_USERNAME + 1), sizeof(char));
    curr_task->hashed_pwd = calloc((MAX_PASSWORD_HASH + 1), sizeof(char));
    curr_task->known_pwd = calloc((MAX_USERNAME + MAX_PASSWORD_HASH + 1), sizeof(char));

    curr_password = calloc((MAX_USERNAME + MAX_PASSWORD_HASH + 1), sizeof(char));
}

void create_curr_task(char *username, char *hashed_pwd, char *known_pwd)
{
    strcpy(curr_task->username, username);
    strcpy(curr_task->hashed_pwd, hashed_pwd);
    strcpy(curr_task->known_pwd, known_pwd);
}

void *solve_task(void *arg)
{
    size_t tid = (size_t)arg;

    struct crypt_data cdata;
    cdata.initialized = 0;

    while (TRUE)
    {
        pthread_barrier_wait(&barrier); // `tid` should wait for the other THREAD_COUNT thread to be able to start together
        if (task_done)
            break; // If the current task was finished by a different thread, just stop

        int hashes = 0;
        char *hashed = NULL;

        // variables that will be passed to `getSubrange()`
        long count = 0;
        long start_index = 0;

        char *pwd = strdup(curr_task->known_pwd);
        int dotsStart = getPrefixLength(curr_task->known_pwd);
        int dotsLength = strlen(pwd) - dotsStart;
        getSubrange(dotsLength, THREAD_COUNT, tid, &start_index, &count);
            setStringPosition(pwd + dotsStart, start_index); // known_pwd = 'xbc....' => 'xbcabcd'

        v2_print_thread_start(tid, curr_task->username, start_index, pwd);

        for (long c = 0; c < count; ++c)
        {
            hashed = crypt_r(pwd, "xx", &cdata);
            hashes++;

            if (strcmp(hashed, curr_task->hashed_pwd) == 0) // found
            {
                strcpy(curr_password, pwd);

                pthread_rwlock_wrlock(&foundLock);
                found = 1;
                pthread_rwlock_unlock(&foundLock);

                v2_print_thread_result(tid, hashes, 0);
                pthread_mutex_lock(&mtx);
                total_hashes += hashes;
                pthread_mutex_unlock(&mtx);

                break;
            }
            else if (found) // by a diff. thread, we should get cancelled
            {
                v2_print_thread_result(tid, hashes, 1);
                pthread_mutex_lock(&mtx);
                total_hashes += hashes;
                pthread_mutex_unlock(&mtx);
                break;
            }

            incrementString(pwd);
        }

        if (!found) // (end)
        {
            v2_print_thread_result(tid, hashes, 2);
            pthread_mutex_lock(&mtx);
            total_hashes += hashes;
            pthread_mutex_unlock(&mtx);
        }
    
        free(pwd);
        pthread_barrier_wait(&barrier); // wait for the other THREAD_COUNT - 1 threads to finish this job
    }

    return NULL;
}

int start(size_t thread_count)
{
    // TODO your code here, make sure to use thread_count!
    // Remember to ONLY crack passwords in other threads

    THREAD_COUNT = thread_count;
    pthread_barrier_init(&barrier, NULL, THREAD_COUNT + 1); // also take into account the `main` thread

    // Create `thread_count` threads
    pthread_t tids[thread_count];
    for (size_t i = 0; i < thread_count; ++i)
    {
        pthread_create(tids + i, NULL, solve_task, (void *)(i + 1)); // tids are 1-indexed !
    }

    char *lineptr = NULL;
    size_t n = 0;
    ssize_t nbytes = 0;

    allocate_curr_task();

    while ((nbytes = getline(&lineptr, &n, stdin)))
    { // read from STDIN ONE TASK

        if (nbytes == -1)
            break;
        if (nbytes > 0 && lineptr[nbytes - 1] == '\n') // remove '\n' appended by getline
            lineptr[nbytes - 1] = '\0';

        char *username = NULL;
        char *hashed_pwd = NULL;
        char *known_pwd = NULL;
        parse_line(lineptr, &username, &hashed_pwd, &known_pwd);
        create_curr_task(username, hashed_pwd, known_pwd);

        double start_wall_clock_time = getTime();
        double start_cpu_time = getCPUTime();
        v2_print_start_user(curr_task->username);

        pthread_barrier_wait(&barrier); // `main` thread blocks until all THREAD_COUNT threads started their job on `curr_task`

        pthread_barrier_wait(&barrier); // `main` waits until all THREAD_COUNT threads finished their job on `curr_task`

        int result = (!found);
        double finish_wall_clock_time = getTime() - start_wall_clock_time;
        double finish_cpu_time = getCPUTime() - start_cpu_time;
        v2_print_summary(curr_task->username, curr_password, total_hashes, finish_wall_clock_time, finish_cpu_time, result);

        pthread_rwlock_wrlock(&foundLock);
        found = 0;
        pthread_rwlock_unlock(&foundLock);

        pthread_mutex_lock(&mtx);
        total_hashes = 0;
        pthread_mutex_unlock(&mtx);
    }
    free(lineptr);
    task_done = TRUE;

    pthread_barrier_wait(&barrier); // 'main` should block until all THREAD_COUNT workers finishde their job

    for (int i = 0; i < THREAD_COUNT; ++i) // all threads must join main in the end
        pthread_join(tids[i], NULL);

    free(curr_password);
    task_destroy(curr_task);
    pthread_barrier_destroy(&barrier);

    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}
