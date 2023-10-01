/**
 * malloc
 * CS 341 - Fall 2023
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct node {
  size_t size; // size of node
  int is_free; // 1, if free, 0 else
  struct node *prev; // pointer to prev node
  struct node *next; // pointer to next node
};

typedef struct node node_t;

static size_t SPLIT_TRESHOLD =
    1024; // Only split a block if it "is worth it" <=> diff. in prev v.s new
          // size >= SPLIT_TRESHOLD

// HEAD: maintains a LL ordered by adresses
static node_t *head = NULL; // TODO: later impl a free list, now: all nodes in
                            // the same list marked `is_free` or not;
// Similar to `mini_memcheck` lab, make use of:
static size_t total_memory_requested =
    0; // current memory in-use by USER from the heap (requested by user
       // throughout the lifetime of the program; Keeps track of how much memory
       // the user holds at any given point in time (i.e.: it may have used
       // `sbrk` for more memory, but freed over time, thus total_mem_requested
       // <= total_mem_sbrk
static size_t total_memory_sbrk = 0; // current memory requested from kernel
// by calling syscall `sbrk`

/*
 * Try to split `node_t *node` into two nodes
 * Has side effects: after the call (if succesful), node should have size
 * `size`, while the other node the remaining size (node_prev_size - size -
 * metdata) Neigh inserted before node Return 1 if the split was succesful, or 0
 * otherwise
 */
int split_succ(size_t size, node_t *node) {

  if (node->size >= 2 * size && // make sure we don't access a next node that is not owned by us
      node->size - size >= SPLIT_TRESHOLD) { // First check needed, or else:
                                             // unsigned int overflow
    node_t *neigh = (node_t*)((char *)(node + 1) + size);
    neigh->is_free = 1;
    neigh->next =
        node; // neigh has a "later" address than node => insert it to its left
              // (remember: LL is ordered by addresses in decreasing order, so
              // that we can keep top of the heap in head
    neigh->size = node->size - (size + sizeof(node_t));
    if (node->prev == NULL) {
      head = neigh;
    } else {
      node->prev->next = neigh;
    }

    neigh->prev = node->prev;
    node->prev = neigh;

    node->size = size;
    return 1;
  } else
    return 0;
}

void coalesce_prev(node_t *node) { // Merge node & node->prev into `node`
  node->size += node->prev->size + sizeof(node_t);
  node->prev = node->prev->prev;
  if (node->prev == NULL)
    head = node;
  else
    node->prev->next = node;
}

node_t *
coalesce_next(node_t *node) { // Merge node & node->next into `node->next`
  node->next->size += node->size + sizeof(node_t);
  node->next->prev = node->prev;
  if (node->prev == NULL)
    head = node->next;
  else
    node->prev->next = node->next;

  return node->next;
}

/*
 * Try to coaleste current free node `node` with its neighbours, both prev and
 * next If both are free => coalesce all three blocks If only one out of two is
 * free (prev or next) -> coalesce w/ that one If neither is free -> NOP
 */
void coalesce_free_neighbours(node_t *node) {

  if (node->prev && node->prev->is_free) {
    coalesce_prev(node);
    //total_memory_requested += sizeof(node_t);
  }
  if (node->next && node->next->is_free) {
    node = coalesce_next(node);
    //total_memory_requested += sizeof(node_t);
  }
}

/**
 * Allocate space for array in memory
 *
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 *
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size) {
  // implement calloc!
  size_t nbytes = num * size;
  char *ptr = malloc(nbytes);
  if (ptr == NULL)
    return NULL;
  else {
    memset(ptr, 0, nbytes); // zero out the bytes;
    return ptr;
  }
}

/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
void *malloc(size_t size) {
  // implement malloc!
  //

  if (size <= 0) // don't allocate - @see
                 // http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
    return NULL;

  node_t *walk = head;
  node_t *winner = NULL;
  int winner_found = 0;

  size_t heap_memory_available = total_memory_sbrk - total_memory_requested;
  if (heap_memory_available >= size) // try to allocate a new node_t w/ `size`
                                     // without requesting from kernel again :D: if there is a large-enough free block 
  {
    // First-fit; TODO: Try other strategies for performance
    while (walk && !winner_found) {
      if (walk->is_free && walk->size >= size) {
        winner = walk;
        winner_found = 1;
        split_succ(size, walk);
        //if (split_succ(size,
          //             walk)) // Splitted `walk` into two nodes: i.) new `walk`
                              // -> which holds exactly `size` and is not free,
                              // and ii.) new `free_node` -> which holds
                              // (prev_size - size - metadata) and is free => we
                              // use extra `metadata` memory from heap as a
                              // trade-off against internal fragmentation
          //total_memory_requested -= sizeof(node_t);
      }
      walk = walk->next;
    }
  }
  if (winner_found) {
    winner->is_free = 0;
    total_memory_requested += winner->size;
  } else { // do `sbrk` :( (SLOW)

    if (head && head->is_free) // As HEAD contains the top of the heap, we can
                               // try to enlarge it with the extra_size needed
                               // => reduce internal fragmentation
    {
      size_t extra_size = size - head->size;
      if (sbrk(extra_size) == (void *)-1) // sbrk failed
        return NULL;
      total_memory_sbrk += extra_size;
      head->size += extra_size;
      head->is_free = 0; // not free anymore, we've just aquired it
      winner = head;
      total_memory_requested += size;
    } else {
      // Allocate a new node_t of size `size`
      winner = sbrk(sizeof(node_t) + size);
      if (winner == (void *)-1)
        return NULL; // `sbrk` failed

      winner->is_free = 0;
      winner->size = size;
      winner->next =
          head; // Insert to the start of the list => make it the new HEAD, as
                // it is the top of the heap now (after `sbrk`)

      if (head != NULL) {
        head->prev = winner;
      } else {
        winner->prev = NULL;
      }

      head = winner;
      total_memory_requested += size;
      total_memory_sbrk += size;
    }
  }

  return winner + 1; // RETURN: usabele memory (to user)
}

/**
 * Deallocate space in memory
 *
 * A block of memory previously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr) {
  // implement free!
  if (ptr == NULL)
    return;

  node_t *node = (node_t *)ptr - 1;
  if (node->is_free)
    return; // (!) Already freed, do not double free

  node->is_free = 1;
  total_memory_requested -= node->size;
  coalesce_free_neighbours(node);
}

/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size) { 
  // implement realloc!

  if (ptr == NULL)
    return malloc(size);
  else if (size == 0) {
    free(ptr);
    return NULL;
  }
 
  node_t *node = (node_t *)ptr - 1; // !!! for `node` user already requested `node->size` memory in the past
  
  if (split_succ(size, node)) {
    node_t *neigh = node->prev; // See contract of `split_succ` -> neigh
                                // inserted before node
     total_memory_requested -=
        neigh->size; // This one is free for use now for a diff. user
  }

  if (size <= node->size)
    return ptr; // Trade-off: internal fragmentation so that we do not
                // complicate it anymore

  node_t *neigh = node->prev;
  if (neigh && neigh->is_free &&
      node->size + (neigh->size + sizeof(node_t)) >=
          size) // can coalesce w/ before in order to `realloc` in these 2
                // merged blocks
  {
    coalesce_prev(node); // node w/ neighbor in `node`
    total_memory_requested +=
        neigh->size + sizeof(node_t); // We make use of previous `neigh->size` allocated but
                     // unused mem
    return node + 1;
  }
  else { // No alternative left, other than moving data from `node_t *node` to a
         // newly created node
    node_t *new_node = malloc(size);
    memcpy(new_node, ptr,
           node->size); // last 'new_size - node->size" values are indeterminate
    free(ptr);          // Check ED: Question that I've asked about `realloc`
    return new_node;
  }
}

// ------------------ HELPER FUNCTIONS ------------------
