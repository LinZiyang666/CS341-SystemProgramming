/**
 * critical_concurrency
 * CS 341 - Fall 2023
 */
#include "barrier.h"
#include <pthread.h>

// The returns are just for errors if you want to check for them.
int barrier_destroy(barrier_t *barrier) {
  int error = 0;
  pthread_mutex_destroy(&barrier->mtx);
  pthread_cond_destroy(&barrier->cv);
  return error;
}

int barrier_init(barrier_t *barrier, unsigned int num_threads) {
  int error = 0;
  barrier->count = 0;
  barrier->times_used = 1; // `closed` barrier
  barrier->n_threads = num_threads;
  pthread_cond_init(&barrier->cv, NULL);
  pthread_mutex_init(&barrier->mtx, NULL);
  return error;
}

int barrier_wait(barrier_t *barrier) {

  pthread_mutex_lock(&barrier->mtx); // Thread arrives at barrier

  while (!barrier->times_used) // `open` barrier beforehand -> wait for the previous threads to go, then the barrier will close for you to gather with the other threads; you are in it `i + 1`
    pthread_cond_wait(&barrier->cv, &barrier->mtx);

  // Barrier is closed now (barrier->times_used == 1)
  barrier->count ++; // Count thread as waiting at the barrier
 
  if (barrier->count == barrier->n_threads) { // Last thread to wait => open barrier and let the current threads in it `i` to go
      barrier->times_used = 0; // Open barrier
      barrier->count --; // Curr. thread is ready to go as the barrier opened
      pthread_cond_broadcast(&barrier->cv); // Let the other `nthreads - 1` threads know that the barrier has opened, so that they can go w/ you
  }
  else  {
      while (barrier->count < barrier->n_threads && barrier->times_used) // If you are one of the `nthreads - 1` threads waiting in front of a CLOSED barrier
        pthread_cond_wait(&barrier->cv, &barrier->mtx);// Just wait :(
      
      // You reached this point after being woken up by broadcast on LoC. 39 -> you are part of iteration `i` and have to leave now, together (in the same batch) with the thread that woke you up
      barrier->count --; 
      if (barrier->count == 0) // Lsat thread to leave -> close barrier before you, as the iteration `i` just finished, and all the following threads in iteration `i+1` will have to wait, you are from different batches
        barrier->times_used = 1; // Close barrier

      pthread_cond_broadcast(&barrier->cv); // Let the next threads know that the barrier has closed now, so they have to wait in front of barrier.
    }
  
   pthread_mutex_unlock(&barrier->mtx);
    
   return 0;
}
