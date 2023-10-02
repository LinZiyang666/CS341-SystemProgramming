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
  struct node *prev_free; // ptr to prev node in free list
  struct node *next_free; // ptr to prev node in free list
};
typedef struct node node_t;

struct btag {
  size_t size; 
};
typedef struct btag btag_t;

static size_t SPLIT_TRESHOLD =
    1024; // Only split a block if it "is worth it" <=> diff. in prev v.s new
          // size >= SPLIT_TRESHOLD

static node_t *head_free = NULL; // free list head
static node_t *tail_mem =
    NULL; // IDEA: ptr to metadata of last node allocated in mem, so that we
          // don't have to call `sbrk(0)` SLOW each time

static node_t *start_mem = NULL;

// Declare functions
// CONTRACT: all functions that contain `free` use a node from the free list;
// all functions that contain `new` allocate memory from kernel using `sbrk`
//

// Request `sizeof(node_t) + size` bytes from kernel using `sbrk` sys. call to
// extend the heap, and allocate a new node of `size` bytes of data; gets called
// if `alloc_free` was not available to call (i.e.: there was no free block in
// the free list that could accomodate `size` bytes of data)
void *alloc_new(size_t size);
// Remove the free node `node` from the free list, and allocate `size` bytes
// into it; If possible, do splitting before allocating this free node, to
// reduce the internal memory fragmentation (see: `alloc_free_split`)
void *alloc_free(size_t size, node_t *node);
/*
 * "insert" extra_node between node and next_node; the insert is implicit, as
 * this happens in the memory / heap (implicit LL of contiguous blocks of
 * memory)
 */
void alloc_free_split(size_t size, node_t *node);
// Remove the `node` from the free list
void remove_free(node_t *node);
// Insert `node` to the head of the free list
void insert_free(node_t *node);

// `prev_node` and `next_node` are valid and free, `node` is a node that is
// willing to be freed => Coalesce prev_node, node and next_node into
// `prev_node`, combining the three small contiguous pieces of memory into a
// larger piece of memory
void coalesce_three_blocks(node_t *node, node_t *prev_node, node_t *next_node);

// As `next_node` is not valid or not free, coalesce `prev_node` and `node` into
// `prev_node`; Combine two small pieces of memory into a larger piece of memory
void coalesce_two_blocks_before(node_t *node, node_t *prev_node,
                                node_t *next_node);
/* coalesce node and node_next by making one
free node in `node`, remove `node_next`
from free list, as it will be combined in
node from now on => `node_next` does not
exist by itself anymore
*/
void coalesce_two_blocks_after(node_t *node,
                               node_t *next_node);

node_t *get_next_mem (node_t *node);
node_t *get_prev_mem (node_t *node); 

node_t *get_next_mem(node_t *node) {
  char *p = (char*)node;
  node_t *cand = (node_t*)(p + sizeof(node_t) + node->size + sizeof(btag_t));
  if (cand > tail_mem) return NULL;
  else return cand;
}

node_t *get_prev_mem (node_t *node) { 
  char *p = (char*)node;
  btag_t *start_btag_prev = (btag_t*)(p - sizeof(btag_t));
  char *data_prev = (char*)start_btag_prev - start_btag_prev->size;
  node_t *prev_mem = (node_t*)(data_prev - sizeof(node_t));
  if (prev_mem < start_mem) return NULL;
  else return prev_mem;
}

void coalesce_three_blocks(node_t *node, node_t *prev_node, node_t *next_node) {
  remove_free(next_node);
  node_t *next_next_node = get_next_mem(next_node);
  if (!next_next_node)
    tail_mem = prev_node;

  prev_node->size += 2 * sizeof(node_t) + 2 * sizeof(btag_t) + node->size + next_node->size;
  btag_t *next_node_btag = (btag_t*)((char*)(next_node + 1) + next_node->size);
  next_node_btag->size = prev_node->size;
}

void coalesce_two_blocks_before(node_t *node, node_t *prev_node,
                                node_t *next_node) {
  if (!next_node)
    tail_mem = prev_node;
  prev_node->size += sizeof(node_t) + sizeof(btag_t) + node->size;
  btag_t *node_btag = (btag_t*)((char*)(node + 1) + node->size);
  node_btag->size = prev_node->size;
}

void coalesce_two_blocks_after(node_t *node,
                               node_t *next_node) {
  remove_free(next_node);
  node_t *next_next_node = get_next_mem(next_node);
  if (!next_next_node)
    tail_mem = node;

  node->is_free = 1;
  node->size += sizeof(node_t) + sizeof(btag_t) + next_node->size;
  btag_t *next_node_btag = (btag_t*)((char*)(next_node + 1) + next_node->size);
  next_node_btag->size = node->size;
  insert_free(node);
}

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
  node_t *node = sbrk(sizeof(node_t) + size + sizeof(btag_t));
  if (node == (void *)-1)
    return NULL;
  else if (!start_mem) // get bottom of heap
    start_mem = node;

  node->prev_free = node->next_free = NULL;
  if (!tail_mem)
    tail_mem = node;

  node->size = size;
  node->is_free = 0;
  btag_t *node_btag = (btag_t*)((char*)(node + 1) + node->size);
  node_btag->size = size;

  return (node + 1);
}

void alloc_free_split(size_t size, node_t *node) {
  node_t *extra_node = (node_t *)((char *)node + sizeof(node_t) + size + sizeof(btag_t));
  //node_t *next_node = get_next_mem(node);

  // Update the new splitted sizes
  extra_node->size = node->size - sizeof(node_t) - size - sizeof(btag_t);
  extra_node->is_free = 1;
  btag_t *extra_node_btag = (btag_t*)((char*)(extra_node + 1) + extra_node->size);
  extra_node_btag->size = extra_node->size;

  if (extra_node > tail_mem) tail_mem = extra_node; // EDGE CASE: split last block, update top of heap

  node->size = size; // Taken
  node->is_free = 0; // redundant, just for readability
  btag_t *node_btag = (btag_t*)((char*)(node + 1) + node->size);
  node_btag->size = size;

  insert_free(extra_node);
}

void *alloc_free(size_t size, node_t *node) {
  if (node->size >=
      sizeof(node_t) + size + sizeof(btag_t) + 
          SPLIT_TRESHOLD) { // split !; // See `malloc_split` diagram Notability
    alloc_free_split(size, node);
  }
  node->is_free = 0;
  return (node + 1);
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

  node_t *node = (node_t*)ptr - 1;
  if (node->is_free)
    return; // (!) Already freed, do not double free

  // Coalesce MEMORY blocks, if neighbours (True neighbours, those adjacent in
  // memory) are free
  node_t *prev_node = get_prev_mem(node);
  node_t *next_node = get_next_mem(node);

  int is_prev_neigh_valid =
      (prev_node && prev_node >= start_mem && prev_node->is_free);
  int is_next_neigh_valid =
      (next_node && next_node <= tail_mem && next_node->is_free);

  if (is_prev_neigh_valid && is_next_neigh_valid) {
    coalesce_three_blocks(node, prev_node, next_node);
  } else if (is_prev_neigh_valid) {
    coalesce_two_blocks_before(node, prev_node, next_node);
  } else if (is_next_neigh_valid) {
    coalesce_two_blocks_after(node, next_node);
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

  node_t *node = (node_t*)ptr - 1; // !!! for `node` user already requested
                                    // `node->size` memory in the past

  if (size <= node->size)
    return ptr; // Trade-off: internal fragmentation so that we do not
                // complicate it anymore

  node_t *reallocd_data =
      malloc(size); // NOTE: this mem. addres hold real data, not the metada
  if (reallocd_data) {
    memcpy(reallocd_data, ptr, node->size);
    free(ptr); // See Edstem Question asked by me -> free `ptr` when memory move
               // is done
  }
  return reallocd_data;
}

// ------------------ HELPER FUNCTIONS ------------------
