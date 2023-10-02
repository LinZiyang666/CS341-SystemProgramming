/**
 * malloc
 * CS 341 - Fall 2023
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct node {
  size_t size;           // size of node
  char is_free;          // 1, if free, 0 else
  struct node *prev_mem; // pointer to prev node in memory (heap, contiguous)
  struct node *next_mem; // pointer to next node in memory (heap, contiguous)
  // TODO: ptr arith to remove prev_mem and next_mem
  struct node *prev_free; // ptr to prev node in free list
  struct node *next_free; // ptr to prev node in free list
};

typedef struct node node_t;

static size_t SPLIT_TRESHOLD =
    1024; // Only split a block if it "is worth it" <=> diff. in prev v.s new
          // size >= SPLIT_TRESHOLD

static node_t *head_free = NULL; // free list head
static node_t *tail_mem =
    NULL; // IDEA: ptr to metadata of last node allocated in mem, so that we
          // don't have to call `sbrk(0)` SLOW each time

// Declare functions
// CONTRACT: all functions that contain `free` use a node from the free list;
// all functions that contain `new` allocate memory from kernel using `sbrk`
void *alloc_new(size_t size);
void *alloc_free(size_t size, node_t *node);
void alloc_free_split(size_t size, node_t *node);
void remove_free(node_t *node);
void insert_free(node_t *node);

void insert_free(node_t *node) {
  if (head_free == NULL) {
    node->prev_free = node->next_free = NULL;
    head_free = node;
    return;
  }

  node->next_free = head_free;
  node->prev_free = NULL;
  head_free->prev_free = node;
  head_free = node;
}

void remove_free(node_t *node) {

  if (node == head_free) {
    head_free = head_free->next_free;
    return;
  }

  node_t *prev_free = node->prev_free;
  node_t *next_free = node->next_free;

  if (prev_free)
    prev_free->next_free = next_free;
  if (next_free)
    next_free->prev_free = prev_free;

  node->prev_free = node->next_free =
      NULL; // Redundant, best practice (clean after yourself)
}

void *alloc_new(size_t size) {
  node_t *node = sbrk(sizeof(node_t) + size);
  if (node == (void *)-1)
    return NULL;

  node->prev_free = node->next_free = NULL;
  node->next_mem = NULL;
  node->prev_mem = tail_mem;
  if (tail_mem)
    tail_mem->next_mem = node;
  tail_mem = node;

  node->size = size;
  node->is_free = 0;

  return node + 1;
}

void alloc_free_split(size_t size, node_t *node) {
  // "insert" extra_node between node and extra_node; the insert is implicit, as
  // this happens in the memory / heap (implicit LL of contiguous blocks of
  // memory)
  node_t *extra_node = (node_t *)((char *)(node + 1) + size);
  node_t *next_node = node->next_mem;

  extra_node->next_mem = next_node;
  if (next_node)
    next_node->prev_mem = extra_node;

  node->next_mem = extra_node;
  extra_node->prev_mem = node;

  // Update the new splitted sizes
  extra_node->size = node->size - sizeof(node_t) - size; // Free
  extra_node->is_free = 1;
  node->size = size; // Taken
  node->is_free = 0; // redundant, just for readability

  insert_free(extra_node);
}

void *alloc_free(size_t size, node_t *node) {
  if (node->size >=
      sizeof(node_t) + size +
          SPLIT_TRESHOLD) { // split !; // See `malloc_split` diagram Notability
    alloc_free_split(size, node);
  }
  node->is_free = 0;
  return node + 1;
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

  node_t *walk = head_free;

  while (walk != NULL) { // First-fit by traversing the FREE LIST
    if (walk->size >= size) {
      remove_free(walk);
      break;
    }
    walk = walk->next_free;
  }

  node_t *data = NULL;
  if (!walk) // We did not find a free node that can accomodate `size` bytes
    data = alloc_new(size);
  else 
    data = alloc_free(size, walk);
  return data;
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

  // Coalesce MEMORY blocks, if neighbours (True neighbours, those adjacent in
  // memory) are free
  node_t *prev_node = node->prev_mem;
  node_t *next_node = node->next_mem;

  int is_prev_neigh_valid = (prev_node && prev_node -> is_free && ((char*)prev_node + sizeof(node_t) + prev_node->size) == (char *)node);
  int is_next_neigh_valid = (next_node && next_node -> is_free && ((char*)node + sizeof(node_t) + node->size) == (char *)next_node);

  if (is_prev_neigh_valid && is_next_neigh_valid) { // coalesce all three blocks: prev, node and next

    remove_free(next_node);
    node_t *next_next_node = next_node->next_mem;
    prev_node->next_mem = next_next_node;
    if (next_next_node)
      next_next_node->prev_mem = prev_node;
    else // next_node is last node in mem, but now was "coalesced" w/ node and
         // prev_node => prev_node is new last node in mem
      tail_mem = prev_node;

    prev_node->size += 2 * sizeof(node_t) + node->size + next_node->size;
  } else if (is_prev_neigh_valid) // coalesce only prev_node and node into prev_node
  {                         // TODO:
    prev_node->next_mem = next_node;
    if (next_node)
      next_node->prev_mem = prev_node;
    else
      tail_mem = prev_node;
    prev_node->size += sizeof(node_t) + node->size;
  } else if (is_next_neigh_valid) { // coalesce node and node_next by making one free node in `node`, remove `node_next` from free list, as it will be combined in node from now on => `node_next` does not exist by itself anymore
    remove_free(next_node);
    node_t *next_next_node = next_node->next_mem;
    node->next_mem = next_next_node;
    if (next_next_node) next_next_node -> prev_mem = node;
    else tail_mem = node;

    node->is_free = 1;
    node->size += sizeof(node_t) + next_node -> size;
    insert_free(node);
  } else { // Coalescing was not possible -> just free `node`
    node->is_free = 1;
    insert_free(node);
  }
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

  node_t *node = (node_t *)ptr - 1; // !!! for `node` user already requested
                                    // `node->size` memory in the past

  if (size <= node->size)
    return ptr; // Trade-off: internal fragmentation so that we do not
                // complicate it anymore

  node_t *reallocd_node = malloc(size); 
  memcpy(reallocd_node, ptr, node->size);
  free(ptr); // See Edstem Question asked by me -> free `ptr` when memory move is done
  return reallocd_node;
}

// ------------------ HELPER FUNCTIONS ------------------
