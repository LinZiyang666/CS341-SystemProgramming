/**
 * mapreduce
 * CS 341 - Fall 2023
 */
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>


int main(int argc, char **argv) {

    if (argc != 6) {
        print_usage(); 
        exit(1);
    }

    int mapper_count = atoi(argv[5]);

    // 1. Create an input pipe for each mapper.

    int *mp[mapper_count]; // `mapper_count` pipes; each pipe will be dynamically allocd. mem. for 2 ints
    for (int i = 0; i < mapper_count; ++i) {
        mp[i] = (int*) malloc(2 * sizeof(int)); // int pipe[2]
        pipe(mp[i]); // Create an input pipe for each mapper
    }

    // Create one input pipe for the reducer.
    int red[2];
    pipe (red);

    // Open the output file.
    char *out_file = argv[2];
    FILE *writer = fopen(out_file, "w");

    // Start a splitter process for each mapper. //TODO: ask lab if ok if reversed
    char *in_file = argv[1];
    pid_t splitter_procs[mapper_count];
    for (int i = 0; i < mapper_count; ++i) {
        splitter_procs[i] = fork();
        if (splitter_procs[i] == 0) // child
        {
            close(mp[i][0]); // close read listening of pipe for Mapper

            // Splitteer should write to the pipe, so that in the next step: the pipe will read from it
            // => stdout of splitter is sent to stdin of mapper
            dup2(mp[i][1], 1);

            // in_file, mapper_count_str, i_str
            char *mapper_count_str = argv[5]; 
            char *i_str = NULL;
            sprintf(i_str, "%d", argv[1]);

            execlp("./splitter", "./splitter", in_file, mapper_count_str, i_str, NULL);
            exit(-1);
        }
    }

    // Start all the mapper processes.
    char *mapper_exec = argv[3];
    pid_t mapper_procs[mapper_count];
    for (int i = 0; i < mapper_count; ++i) {
        close(mp[i][1]); // close write listening BEFORE forking
        mapper_procs[i] = fork(); 
        if(mapper_procs[i] == 0) // child
         {
            close(red[0]);
            dup2(mp[i][0], 0); 
            dup2(red[1], 1); 

            execlp(mapper_exec, mapper_exec, NULL);
            exit(-1);
         }
        
    }


    // Start the reducer process.
    close(red[1]); // close write ending of Reducer BEFORE forking
    char *reducer_exec = argv[4];
    pid_t red_proc = fork(); 
    if (red_proc == 0) // child
    {
        dup2(red[0], 0);
        int out_fd = fileno(out_file);
        dup2(out_fd, 1);
        execlp(reducer_exec, reducer_exec, NULL);
        exit(-1);
    }

    // Wait for the reducer to finish.
    for (int i = 0; i < mapper_count; ++i) // splitters
    {
        int status;
        waitpid(splitter_procs[i], &status, NULL);
    }

    for (int i = 0; i < mapper_count; ++i) // mappers
    {
        int status;
        waitpid(mapper_procs[i], &status, NULL);
    }
    
    int status;
    waitpid(red_proc, &status, NULL);

    // Print nonzero subprocess exit codes.
    if (status)
        print_nonzero_exit_status(reducer_exec, status);

    // Count the number of lines in the output file.
    print_num_lines(out_file);


    // Free resources
    fclose(writer); 
    close(red[0]);
    for (int i = 0; i < mapper_count; ++i) {
        free(mp[i]);
    }
    return 0;
}
