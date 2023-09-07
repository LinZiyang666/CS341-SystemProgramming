/**
 * utilities_unleashed
 * CS 341 - Fall 2023
 */
#include "format.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{

    if (argc < 2)
        print_time_usage();

    struct timespec tp_start;
    int start = clock_gettime(CLOCK_MONOTONIC, &tp_start);
    if (start == -1)
    {
        perror("Starting the clock failed");
        exit(1);
    }

    pid_t child = fork();
    if (child == -1)
    {
        print_fork_failed();
    }
    else if (child == 0)
    { // child
        execvp(argv[1], argv + 1);
        print_exec_failed();
    }

    int status;
    waitpid(child, &status, 0);

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) { // Exited correctly

    struct timespec tp_end;
    int end = clock_gettime(CLOCK_MONOTONIC, &tp_end);

    if (end == -1)
    {
        perror("Starting the clock failed");
        exit(1);
    }

    double duration = (tp_end.tv_sec - tp_start.tv_sec) + (tp_end.tv_nsec - tp_start.tv_nsec) * 1.0 / 1e9;
    display_results(argv, duration);
    return 0;

    }

}
