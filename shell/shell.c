/**
 * shell
 * CS 341 - Fall 2023
 */
#include "format.h"
#include "shell.h"
#include "vector.h"
#include <unistd.h>

typedef struct process
{
    char *command;
    pid_t pid;
} process;

static vector *processes;
static int h_flag = 0;
static int f_flag = 0;
static FILE *file = NULL; // Our input stream, may be `stdin` or a file if `-f` is specified

static char *history_path = NULL;

void start_shell(int argc, char *argv[], vector *history)
{

    int expected_argc = 1; // program name
    int opt = 0;
    char *h_name = NULL;
    char *f_name = NULL;

    while ((opt = getopt(argc, argv, "h:f:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            h_flag = 1;
            h_name = optarg;
            expected_argc += 2;
            break;
        case 'f':
            f_flag = 1;
            f_name = optarg;
            expected_argc += 2;
            break;
        default:
            break;
        }
    }

    if (argc != expected_argc)
    {
        print_usage();
        exit(1);
    }

    if (h_flag)
    { // When provided `-h`, the shell should load in the history file as its history.
        FILE *history_file;

        history_path = get_full_path(h_name);    // TODO: free it (when writing back to history)
        history_file = fopen(history_path, "r"); // TODO: check for failure
        if (!history_file)
        {
            print_history_file_error();
            exit(1);
        }

        char *line = NULL;
        size_t len = 0;
        ssize_t nread;

        while ((nread = getline(&line, &len, history_file) != -1)) // load history
        {
            if (nread > 0 && line[nread - 1] == '\n')
            {
                line[nread - 1] = '\0';
                vector_push_back(history, line);
            }
        }

        free(line);
        fclose(history_file);
    }

    if (f_flag)
    {                                            // Your shell should also support running a series of commands from a script f
    // When provided -f, your shell will both print and run the commands in the file in sequential order until the end of the file
        file = fopen(f_name, "r");
        if (!file) { //non-existent script file
            print_script_file_error(); 
            exit(1);
        }
    }
    else // read from `stdin`
        file = stdin;
}

int shell(int argc, char *argv[])
{
    // This is the entry point for your shell.

    // TODO: handle_signals(); - SIGINT & SIGCHILD

    processes = shallow_vector_create(); // to store info later about processes
    vector *history = string_vector_create();
    start_shell(argc, argv, history);

    // TODO: Start shell -> while(1)


    // TODO: Upon exiting, the shell should append the commands of the current session into the supplied history file
    if (h_flag) {
        FILE *output = fopen(history_path, "w");
        VECTOR_FOR_EACH(history, buffer, {
            fprintf(output, "%s\n", (char *) buffer);
        });

        fclose(output); 
        free(history_path);
    }
    
    
    if (f_flag)
        fclose(file);

    return 0;
}
