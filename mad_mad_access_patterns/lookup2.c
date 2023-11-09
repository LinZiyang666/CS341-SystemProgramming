/**
 * mad_mad_access_patterns
 * CS 341 - Fall 2023
 */
#include "tree.h"
#include "utils.h"
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

void search_word(void *ptr, char *target_word, uint32_t off)
{ // `off` is realtive to the start of the file

  if (off == 0)
  {
    printNotFound(target_word);
    return;
  }

  // When reading a node from the memory mapped file, use pointer arithmetic to jump to the correct position and read the node using regular pointer dereferencing.
  BinaryTreeNode *node_ptr = (BinaryTreeNode *)((char *)ptr + off); // cast void* -> char* to be POSIX compliant

  char *curr_word = node_ptr->word;
  if (strcmp(target_word, curr_word) == 0)
  { // match
    printFound(target_word, node_ptr->count, node_ptr->price);
  }
  else if (strcmp(target_word, curr_word) < 0)
  { // search left subtree
    search_word(ptr, target_word, node_ptr->left_child);
  }
  else
  { // search right subtree
    search_word(ptr, target_word, node_ptr->right_child);
  }
}

/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses mmap to access the data.

  ./lookup2 <data_file> <word> [<word> ...]
*/

int main(int argc, char **argv)
{

  if (argc < 1 + 2) // ./lookup on argv[0] -> If run with less than 2 arguments, your program should print an error message describing the arguments it expects and exit with error code 1.
  {
    printArgumentUsage();
    exit(1);
  }

  char *data_file = argv[1];
  int fd = open(data_file, O_RDONLY);
  if (fd == -1)
  {
    openFail(data_file);
    exit(2);
  }

  struct stat statbuf;
  if (fstat(fd, &statbuf) == -1)
  { // May be redundant, but to keep best practices of checking error codes
    openFail(data_file);
    exit(2);
  }

  if (!S_ISREG(statbuf.st_mode)) // not a regular file
  {
    openFail(data_file);
    exit(2);
  }

  void *ptr = mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (ptr == MAP_FAILED)
  { // If a call to mmap fails (for version 2), print a helpful error message and exit with error code 3.
    mmapFail(data_file);
    exit(3);
  }

  if (strncmp(ptr, BINTREE_HEADER_STRING, BINTREE_ROOT_NODE_OFFSET) != 0)
  { // if  the first 4 bytes are not “BTRE”,
    formatFail(data_file);
    exit(2);
  }

  for (int i = 2; i < argc; ++i)
  {
    char *word = argv[i];
    search_word(ptr, word, BINTREE_ROOT_NODE_OFFSET);
  }

  munmap(ptr, statbuf.st_size);
  close(fd);
  

  return 0;
}
