/**
 * deepfried_dd
 * CS 341 - Fall 2023
 */
#include "format.h"
#include <unistd.h>
#include <stdio.h>

size_t full_blocks_in, partial_blocks_in,
    full_blocks_out, partial_blocks_out,
    total_bytes_copied;
double time_elapsed;

FILE *in, *out; 
size_t block_size, block_count; 
size_t p_blocks, k_blocks;

    void
    parse_in(int argc, char **argv)
{

    // i: -> specifies that if parser find flag `i` (-i), then an argument for it is expected (in this case the INPUT_FILENAME)
    // getopt returns `?` if finds flag that is not contained 
    // getopt returns `:` if missing option argument (by default : ret `?`, but we override this behavior w/ first `:` in char *fmt)
    const char *fmt = ":i:o:b:c:p:k:";
    char c;
    while ((c = getopt(argc, argv, fmt)))
    {
        switch (c)
        {
        case 'i':
            in = fopen(optarg, "r");
            if (!in) {
                print_invalid_output(optarg);
                exit(1);
            }
            
        case 'o':
            break;
        case 'b':
            break;
        case 'c':
            break;
        case 'p':
            break;
        case 'k':
            break;
        default:
            exit(1);
        }
    }
}

int main(int argc, char **argv)
{
    parse_in(argc, argv);
    return 0;
}