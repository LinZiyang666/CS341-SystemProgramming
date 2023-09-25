/**
 * malloc
 * CS 341 - Fall 2023
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct node {
  char is_free;      // 1, if free, 0 else
  size_t size;       // size of node
  struct node *prev; // pointer to prev node
  struct node *next; // pointer to next node
};

typedef struct node node_t;

// Try implementing a simpler variant of a binary Buddy Allocator

static size_t FRAGMENT = 4; // TODO: Tweak this param. afterwards

static size_t
    buddy_size[16] = {(1 << 3),  (1 << 4),  (1 << 5),  (1 << 6),  (1 << 7),
                      (1 << 8),  (1 << 9),  (1 << 10), (1 << 11), (1 << 12),
                      (1 << 13), (1 << 14), (1 << 15), (1 << 16), (1 << 17),
                      (1 << 18)}; // buddy_size[i] = head of free list allocator
                                  // associated w/ size 2^(i+3), i in [0, 16);
                                  // allocate all free blocks w/ sz <= 2 ^i in
                                  // buddy_size[i]; if sz > (1 << 18), allocate
                                  // to buddy_size[15] - last cell

static node_t *suballocators_h[16]; // suballocators_h[i] = head of the
                                    // suballocator list for nodes w/ size <=
                                    // 2^i; check `buddy-size` for more info

size_t get_allocator(size_t size);
node_t *allocate_node(size_t size);
node_t *best_fit(size_t size, size_t allocator);
void remove_from_free_list(node_t *node, size_t allocator);
void split_node(node_t *node, size_t new_size);
void insert_front(node_t *node, size_t allocator);

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

  if (size <= 0) // don't allocate - @see
                 // http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
    return NULL;
  size_t allocator = get_allocator(size);

  if (suballocators_h[allocator] ==
      NULL) { // no free node that we can reuse => use `sbrk`
    node_t *ptr = allocate_node(size);
    if (ptr == (void *)-1)
      return NULL;
    return ptr;
  } else {
    node_t *ptr = best_fit(size, allocator);
    if (!ptr) {
      ptr = allocate_node(size);
      return ptr;
    } else {
      remove_from_free_list(ptr, allocator);
      ptr->is_free = 0; // mark as taken
      split_node(ptr, size);
      return ((node_t *)ptr + 1); // Return the (!) user-accessible memory (!)
    }
  }
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
  return NULL;
}

// ------------------ HELPER FUNCTIONS ------------------

/*
  Get the index in the buddy_size C-array, which tells us: what free list (i.e:
  suballocators) to use for the free node w/ this size
*/
size_t get_allocator(size_t size) {

  for (size_t allocator = 0; allocator < 16; ++allocator)
    if (size <= buddy_size[allocator]) // found!
      return allocator;

  return 15; // TODO: Later improve the policy to not allocate all free nodes w/
             // size > (1 << 16) to last allocator
}

/*
 sbrk() to increase the heap size
*/
node_t *allocate_node(size_t size) {
  node_t *node = sbrk(sizeof(node_t) + size);
  if (node == (void *)-1)
    return NULL;
  node->size = size;
  node->is_free = 0; // taken
  node->prev = node->next = NULL;
  return node;
}

/*
 * Search for the best_fit in the suballocator specific to size `size`
 * best_fit = the smallest size node that has enough capacity (node->size >=
 * size) to allow the current node to fit (i.e.: has at least size `size`) In
 * case of tie -> choose the left-most (first one found) one If no node found ->
 * ret. NULL
 */
node_t *best_fit(size_t size, size_t allocator) {
  node_t *node = NULL;

  node_t *suballocator_h = suballocators_h[allocator];
  for (node_t *walk = suballocator_h; walk != NULL; walk = walk->next) {
    if (size <= walk->size) { // candidate
      if (!node || node->size > size)
        node = walk;
    }
  }
  return node;
}

void remove_from_free_list(node_t *node, size_t allocator) {
  node_t *prev_node = node->prev;
  node_t *next_node = node->next;

  if (prev_node == NULL) // it was the head
  {
    suballocators_h[allocator] = NULL;
  } else {
    prev_node->next = next_node;
    if (next_node)
      next_node->prev = prev_node;
  }

  // Clean-up links (redundant, but great practice)
  node->prev = node->next = NULL;
}

/*
* Split the `node` into -> node with `new_size` of data being used (in order to reduce the internal fragmentation) & node `created` that is marked and free and added to the associated suballocator's list
*/
void split_node(node_t *node, size_t new_size) { 
    
    if (node->size >= new_size + sizeof(node_t) + FRAGMENT)  // Only split if we exceed the FRAGMENT treshold, to: not external fragment our memory
    { // We are ensured to write in a memory owned by US
        node_t *created = (node_t *) ((char *)(node + 1) + node -> size);
        created->size = node->size - (new_size + sizeof(node_t));
        created->is_free = 1;

        node->size = new_size;

        size_t allocator = get_allocator(created->size);
        insert_front(created, allocator);
    }
}

/*
* Insert `node` to the front of the suballocator indexed at `allocator` 's list
* If the list is EMPTY -> make `node` the head of the list
*/
void insert_front(node_t *node, size_t allocator) { 
    node_t *suballocator_h = suballocators_h[allocator]; 
    if (suballocator_h == NULL) {
        suballocators_h[allocator] = node;
    }

    node -> prev = NULL;
    node -> next = suballocator_h; 
    suballocator_h -> prev = node; 
    suballocators_h[allocator] = node;
}
