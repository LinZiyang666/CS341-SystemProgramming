/**
 * critical_concurrency
 * CS 341 - Fall 2023
 */
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "queue.h"


/* ----------------- HELPER METHODS ------------------- */ 

void *push(void *arg) {
    queue *q = (queue *) arg;
    queue_push(q, (void *)2);
    return NULL;
}

void *pull(void *arg) {
    queue *q = (queue *) arg;
    printf("q.top() = %ld\n", (size_t) queue_pull(q)); // try to pull before pushing
    return NULL;
}

void t1() {
    queue *q = queue_create(10);
    queue_push(q, (void *)1);
    printf("q.top() = %ld\n", (size_t) queue_pull(q));
    queue_push(q, (void *)2);
    printf("q.top() = %ld\n", (size_t) queue_pull(q));
    queue_push(q, (void *)3);
    printf("q.top() = %ld\n", (size_t) queue_pull(q));

    queue_push(q, (void *) 5);
    queue_push(q, (void *) 6);
    queue_push(q, (void *) 7);
    printf("q.top() = %ld\n", (size_t) queue_pull(q));

    queue_destroy(q);
}

void block_pull_empty() {
    queue *q = queue_create(10);
    queue_push(q, (void *)1);
    printf("q.top() = %ld\n", (size_t) queue_pull(q));

    pthread_t tid1, tid2;
    pthread_create(&tid1, NULL, pull, (void *) q);
    pthread_create(&tid2, NULL, push, (void *) q);

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    queue_destroy(q);
}

void block_push_max_size() {
    queue *q = queue_create(3);
    queue_push(q, (void *)1);
    queue_push(q, (void *)2);
    queue_push(q, (void *)3);

    pthread_t tids[7];
    for (int i = 0; i < 7; ++i) { // 2pushes, 5 pulls
     if (i == 1 || i == 2) 
       pthread_create(&tids[i], NULL, push, (void *) q);
     else 
       pthread_create(&tids[i], NULL, pull, (void *) q);
    }

    for (int i = 0; i < 7; ++i) 
        pthread_join(tids[i], NULL);

    queue_destroy(q);
}

void dont_block_push_neg_max_size() {
    queue *q = queue_create(-1);
    queue_push(q, (void *)1);
    queue_push(q, (void *)2);
    queue_push(q, (void *)3);


    printf("q.top() = %ld\n", (size_t) queue_pull(q)); // try to pull before pushing
   
    queue_destroy(q);
}

int main(int argc, char **argv) {
    /* if (argc != 3) {
        printf("usage: %s test_number return_code\n", argv[0]);
        exit(1);
    }
    printf("Please write tests cases\n"); */

    t1();
    printf("--------------\n");
    block_pull_empty();
    printf("--------------\n");
    block_push_max_size(); 
    printf("--------------\n");
    dont_block_push_neg_max_size();
    printf("--------------\n");



    return 0;
}
