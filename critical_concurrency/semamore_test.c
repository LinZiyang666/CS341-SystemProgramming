/**
 * critical_concurrency
 * CS 341 - Fall 2023
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "semamore.h"

// ************** HELPER FUNCTIONS **************  // 

void *push(void *arg) {
  Semamore *s = (Semamore *)arg;
  semm_post(s);
  printf("I pushed :D\n");
  return NULL;
}

void *pull(void *arg) {
  Semamore *s = (Semamore *)arg;
  semm_wait(s);
  printf("I pulled :D\n");
  return NULL;
}

// ************** HELPER FUNCTIONS **************  // 

void t1() {
  /* A woken thread must acquire the lock, so it will also have to wait until we
   * call unlock*/

  Semamore *s = malloc(sizeof(Semamore));
  semm_init(s, 5, 10);

  printf("s->value: %d\n", s->value);
  printf("s->max_val: %d\n", s->max_val);

  semm_post(s);
  printf("Expected s->value: 6, and got: %d\n", s->value);
  printf("s->max_val should not change; Expected: 10, and got: %d\n",
         s->max_val);

  for (int i = 0; i < 3; ++i)
    semm_wait(s);

  printf("Expected s->value: 3, and got: %d\n", s->value);


  printf("I will destroy the sem now\n");
  semm_destroy(s);
  free(s); // Don't forget to free up the semaphore mem
  printf("---------------\n");
}

void wait_all_block() {
  Semamore *s = malloc(sizeof(Semamore));
  semm_init(s, 5, 10);
  for (int i = 0; i < 5; ++i)
    semm_wait(s);

  printf("Expected s->value: 0, and got: %d\n", s->value);

  pthread_t tid1, tid2;
  pthread_create(&tid1, NULL, pull, (void *)s);
  pthread_create(&tid2, NULL, push, (void *)s);

  pthread_join(tid1, NULL);
  pthread_join(tid2, NULL);

  semm_destroy(s);
  free(s); // Don't forget to free up the semaphore mem
  printf("---------------\n");
}

void post_all_block() {
  Semamore *s = malloc(sizeof(Semamore));
  semm_init(s, 5, 10);
  for (int i = 0; i < 5; ++i)
    semm_post(s);

  printf("Expected s->value: 10 == max->value: 10, and got: %d == %d\n", s->value, s->max_val);

  pthread_t tid1, tid2;
  pthread_create(&tid2, NULL, push, (void *)s);
  pthread_create(&tid1, NULL, pull, (void *)s);

  pthread_join(tid1, NULL);
  pthread_join(tid2, NULL);

  semm_destroy(s);
  free(s); // Don't forget to free up the semaphore mem
  printf("---------------\n");
}

int main(int argc, char **argv) {
  // printf("Please write tests in semamore_tests.c\n");
  t1();
  wait_all_block();
  post_all_block();

  return 0;
}
