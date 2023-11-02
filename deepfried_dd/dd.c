/**
 * deepfried_dd
 * CS 341 - Fall 2023
 */
#include "format.h"
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define FALSE 0
#define TRUE 1

static size_t full_blocks_in, partial_blocks_in,
    full_blocks_out, partial_blocks_out;
static double time_elapsed;

static FILE *in, *out;
static size_t block_size, block_count;
static size_t p_blocks, k_blocks;

static size_t total_bytes_copied;
static int received_sigusr;

static struct timespec start, end;

void set_default_params()
{
    in = stdin;
    out = stdin;
    block_size = 512;
    block_count = -1;
    p_blocks = k_blocks = 0;

    total_bytes_copied = 0;
    received_sigusr = FALSE;

    full_blocks_in = partial_blocks_in = full_blocks_out = partial_blocks_out = 0;
    time_elapsed = 0.0;
}

void print_rep() {
    clock_gettime(CLOCK_REALTIME, &end);
    double seconds_spent = difftime(end.tv_sec, start.tv_sec); // we will have to add the last diff
    double nanoseconds_spent_to_seconds = (end.tv_nsec - start.tv_nsec) * 1.0 / 1e9;
    print_status_report(full_blocks_in, partial_blocks_in, full_blocks_out, partial_blocks_out, total_bytes_copied,
    seconds_spent + nanoseconds_spent_to_seconds); // TODO: ask lab if this is proper way of doing it

    received_sigusr = FALSE; // reset flag to continue execution in `while(1)` busy-wait loop
    //TODO: check lab
}

void parse_in(int argc, char **argv)
{ // Invalid input (i.e: block size that is not a size_t) is considered UB, so don't worry about it

    set_default_params();
    // i: -> specifies that if parser find flag `i` (-i), then an argument for it is expected (in this case the INPUT_FILENAME)
    // getopt returns `?` if finds flag that is not contained
    // getopt returns `:` if missing option argument (by default : ret `?`, but we override this behavior w/ first `:` in char *fmt)
    const char *fmt = ":i:o:b:c:p:k:";
    char c;
    while ((c = getopt(argc, argv, fmt)) != -1) // not finished parsing
    {
        switch (c)
        {
        case 'i':
            in = fopen(optarg, "r");
            if (!in)
            {
                print_invalid_output(optarg);
                // puts("failed opening in file");
                exit(1);
            }
            break;
        case 'o':
            out = fopen(optarg, "w+"); // check notes iPad for explanation OR check docs for `fseek()`
            if (!out)
            {
                print_invalid_output(optarg);
                // puts("failed opening/creating out file");
                exit(1);
            }
            break;
        case 'b':
            block_size = atoi(optarg);
            break;
        case 'c':
            block_count = atoi(optarg);
            break;
        case 'p':
            p_blocks = atoi(optarg);
            break;
        case 'k':
            k_blocks = atoi(optarg);
            break;
        default:
            // puts("failed parsing");
            exit(1);
        }
    }
}

void run_dd()
{

    int blocks_copied = 0;
    char buf[block_size]; // we read block by block that are `block_size` each, until we have read `block_count` blocs
    // zero-out memory
    memset(buf, 0, sizeof(buf));

    while (1)
    {
        if (received_sigusr)
        {
            print_rep();
        }
        size_t bytes_read = fread(buf, 1, block_size, in); // read `block_size` bytes (1) from `in` file and write to `char buf[]`
        if (bytes_read == 0)                               // if we are done reading
            break;                                         // stop

        // Write the read bytes to output file
        fwrite(buf, 1, bytes_read, out); // write `bytes_read` bytes (1) from `char buf[]` to `out` file
        total_bytes_copied += bytes_read;

        blocks_copied++;
        if (block_size != bytes_read) { // we have done the last part of our read, and it was a `partial` one -> there are only `bytes_read` left to read, so we did not read an entire block
            partial_blocks_in ++;
            partial_blocks_out ++;
            break;
        }
        else if (block_count == (size_t) blocks_copied) // we finished copying all blocks
        {
            full_blocks_in++;
            full_blocks_out++;
            break;
        }
        else { // continue reading -> we've just finished reading another block
            full_blocks_in++;
            full_blocks_out++;
        }
    }
}

void sighandler(int signo)
{
    assert(signo == SIGUSR1);
    received_sigusr = TRUE;
}
int main(int argc, char **argv)
{
    clock_gettime(CLOCK_REALTIME, &start);
    signal(SIGUSR1, sighandler);
    parse_in(argc, argv);

    // seek `in` file by `p_blocks` and `out` file by `k_blocks`
    // offsets should be calculated in Bytes

    long in_off = p_blocks * block_size;
    if (in != stdin && in_off)
        if (-1 == fseek(in, in_off, SEEK_SET))
        {

            // puts("failed seek in");
            exit(1);
        }
    long out_off = k_blocks * block_size;

    if (out != stdout && out_off)
        if (-1 == fseek(out, out_off, SEEK_SET))
        {
            // puts("failed seek out");
            exit(1);
        }

    run_dd();
    print_rep();
    fclose(in);
    fclose(out);

    return 0;
}