/**
 * mad_mad_access_patterns
 * CS 341 - Fall 2023
 */
#include "tree.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

void search_word(char* word, FILE* input_file, uint32_t off) { // `off` is relative to the beginning of the file

    if (off == 0) // leaf, base case
      {
        printNotFound(word);
        return;
      }
    
    // Step 0: Get to your current node 
    fseek(input_file, off, SEEK_SET); // SEEK_SET = relative to start of file
 
    // Step 1: Read node's metadata (sizeof(BinaryTreeNode) bytes, just before `char word[]`) form file, at the current position => populate `node` w/ metadata
    BinaryTreeNode node;
    fread(&node, sizeof(BinaryTreeNode), 1, input_file);
    
    char *curr_word = calloc(1, 1024); // TODO: Later do reading until the end of file as in Networking MP
    
    // fseek(input_file, off + sizeof(BinaryTreeNode), SEEK_SET); // position file pointer before `char word[]`, so that we can read the actual `curr_word` ; TODO: Check if needed, as: Each call to fread then advances the file pointer by the number of bytes read.
    fread(curr_word, 1024, 1, input_file);

    if (strcmp(word, curr_word) == 0) // match :D
      {
        printFound(word, node.count, node.price);
        return;
      }
    else if (strcmp(word, curr_word) < 0) { // BST: go search to left

      search_word(word, input_file, node.left_child);
      return;
    }
    else { // BST: go search to right
      search_word(word, input_file, node.right_child);
      return;
    }
}

/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses fseek() and fread() to access the data.

  ./lookup1 <data_file> <word> [<word> ...]
*/

int main(int argc, char **argv)
{

  if (argc < 1 + 2) // ./lookup on argv[0] -> If run with less than 2 arguments, your program should print an error message describing the arguments it expects and exit with error code 1.
  {
    printArgumentUsage();
    exit(1);
  }

  char *data_file = argv[1];
  FILE *input_file = fopen(data_file, "r");
  if (!input_file) // If the data file cannot be read
  {
    openFail(data_file);
    exit(2);
  }

  char BTRE_buff[5];
  fread(BTRE_buff, 1, BINTREE_ROOT_NODE_OFFSET, input_file);
  if (strncmp(BTRE_buff, BINTREE_HEADER_STRING, BINTREE_ROOT_NODE_OFFSET) != 0) //  the first 4 bytes are not “BTRE”
   {
    formatFail(data_file);
    exit(2);
   }


  // if first 4 bytes are not “BTRE”, print a helpful error message and exit with error code 2.

  puts(data_file);
  for (int i = 2; i < argc; ++i)
  {
    char *word = argv[i];
    search_word(word, input_file, BINTREE_ROOT_NODE_OFFSET);
  }

  return 0;
}
