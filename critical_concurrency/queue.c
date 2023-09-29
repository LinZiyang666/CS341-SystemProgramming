/**
 * critical_concurrency
 * CS 341 - Fall 2023
 */
#include "queue.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * This queue is implemented with a linked list of queue_nodes.
 */
typedef struct queue_node {
  void *data;
  struct queue_node *next;
} queue_node;

struct queue {
  /* queue_node pointers to the head and tail of the queue */
  queue_node *head, *tail;

  /* The number of elements in the queue */
  ssize_t size;

  /**
   * The maximum number of elements the queue can hold.
   * max_size is non-positive if the queue does not have a max size.
   */
  ssize_t max_size;

  /* Mutex and Condition Variable for thread-safety */
  pthread_cond_t cv;
  pthread_mutex_t m;
};

queue *queue_create(ssize_t max_size) {
  /* Your code here */
  queue *q = malloc(sizeof(queue));
  q->head = q->tail = NULL;
  q->max_size = max_size;
  q->size = 0;
  pthread_mutex_init(&q->m, NULL);
  pthread_cond_init(&q->cv, NULL);
  return q;
}

void queue_destroy(queue *this) {
  /* Your code here */
  pthread_cond_destroy(&this->cv);
  pthread_mutex_destroy(&this->m);
  queue_node *walk = this->head;
  while (walk != this->tail) {
    // !!!!!! free(walk->data); // WRONG -> deallocate all the storage capacaity allocated by the queue, so only the `queue_node*`, the `void *data` was passed as arg, we used the one allocated by the caller
    queue_node *prev = walk;
    walk = walk->next;
    prev->next = NULL;
    free(prev);
    prev = NULL;
  }

  free(walk); // == this->tail; (!) make sure to not double-free
  free(this);
}

void queue_push(queue *this, void *data) {
  /* Your code here */
  pthread_mutex_lock(&this->m);

  while (
      this->size ==
      this->max_size) // Blocks if the queue is full (defined by it's max_size).
                      // ; if max_size < 0, while cond is always false
    pthread_cond_wait(&this->cv, &this->m);

  // Enque to end (tail)
  queue_node *node = malloc(sizeof(queue_node));
  node->data = data;
  node->next = NULL;

  if (this->tail) {
    this->tail->next = node;
    this->tail = node;
  } else {
    this->head = this->tail = node;
  }

  this->size++;
  pthread_cond_broadcast(&this->cv);
  pthread_mutex_unlock(&this->m);
}

void *queue_pull(queue *this) {
  /* Your code here */

  pthread_mutex_lock(&this->m);
  queue_node *node = NULL;

  while (this->size == 0) // Blocks if the queue is empty.
    pthread_cond_wait(&this->cv, &this->m);

  void *data = this->head->data;
  node = this->head;
  this->head = this->head->next;  
  free(node);
  node = NULL;

  this->size--;

  if (this->size == 0)
    this->head = this->tail = NULL;

  pthread_cond_broadcast(&this->cv);
  pthread_mutex_unlock(&this->m);

  return data;  // TODO: clarify if we return data or node
}
