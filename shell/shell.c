/**
 * shell
 * CS 341 - Fall 2023
 */
#include "format.h"
#include "shell.h"
#include "vector.h"
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

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

void killAllChildProcesses()
{
    size_t n = vector_size(processes);
    for (size_t i = 0; i < n; ++i)
    {
        process *p = (process *)vector_get(processes, i);
        kill(p->pid, SIGKILL);
        // Free the memory taken by each process
        free(p->command);
        free(p);
    }

    vector_destroy(processes);
}

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
        }

        char *line = NULL;
        size_t len = 0;
        ssize_t nread;

        while ((nread = getline(&line, &len, history_file)) != -1) // load history
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
    {
        // When provided -f, your shell will both print and run the commands in the file in sequential order until the end of the file
        file = fopen(f_name, "r");
        if (!file)
        { // non-existent script file
            print_script_file_error();
            exit(1);
        }
    }
    else // read from `stdin`
        file = stdin;
}

// Remove process with PID = <pid>, by freeing it from the memory & removing it from the `processes` vector
void remove_process(pid_t pid)
{
    size_t n = vector_size(processes);

    for (size_t i = 0; i < n; ++i)
    {
        process *p = vector_get(processes, i);
        if (p->pid == pid)
        {
            free(p->command);
            free(p);
            vector_erase(processes, i);
            break;
        }
    }
}
void handle_SIGINT()
{

    size_t n = vector_size(processes);
    for (size_t i = 0; i < n; ++i)
    {
        process *p = vector_get(processes, i);
        if (p->pid != getpgid(p->pid)) // foreground process
        {
            kill(p->pid, SIGKILL);
            remove_process(p->pid);
        }
    }
}

void handle_signals()
{
    signal(SIGINT, handle_SIGINT);
    // TODO: [PART 2] handle SIGCHILD
}

// Return status code of executtion of command
// Here come all the commands that have to log in `history`
int execute_command(char *buffer)
{
    // `cd <path>`
    if (!strncmp(buffer, "cd", 2))
    {
        char *path = buffer + 3;
        int succesfully_chdir = chdir(path);
        if (succesfully_chdir == -1)
        {
            print_no_directory(path);
            return 1;
        }
        else
            return 0;
    }
    // TODO: add other functions

    return 0;
}

int shell(int argc, char *argv[])
{
    // This is the entry point for your shell.

    // Handle the relevant signals for the task
    handle_signals();

    processes = shallow_vector_create(); // to store info later about processes
    vector *history = string_vector_create();
    start_shell(argc, argv, history);

    // Start shell -> while(1)

    char *buffer = NULL;
    size_t len = 0;
    ssize_t nread;

    while ((nread = getline(&buffer, &len, file)) != -1)
    {

        char *cwd = get_full_path("./");
        print_prompt(cwd, getpid()); // When prompting for a command, the shell will print a prompt in the following format (from format.h):
        free(cwd);                   // Ensures any flow control gets here

        // Keep track of commands in history; also - prepare buffer to print: Note the lack of a newline at the end of this prompt.

        if (nread > 0 && buffer[nread - 1] == '\n')
        {
            buffer[nread - 1] = '\0';
            // Your shell should also support running a series of commands from a script f
            if (file != stdin) // As `stdin` already gets displayed to the shell session, we want to display to shell only if '-f/-h' is used to read
                print_command(buffer);
        }

        // TODO: [PART 1] solve built-in commands - Run in shell (main / parent) process, don't `fork()`
        if (!strncmp(buffer, "exit", 4))
            break;
        else if (!strcmp(buffer, "!history"))
        {                                                // !history
            size_t history_lines = vector_size(history); // Print `history` vector line by line
            for (size_t i = 0; i < history_lines; ++i)
                print_history_line(i, (char *)vector_get(history, i));
        }
        else if (buffer[0] == '#')
        {                                              // '#<n>
            size_t command_line_nr = atoi(buffer + 1); // Precondition: "buffer + 1" represents a valid integer: n >= 0
            if (command_line_nr >= vector_size(history))
            { //  If `n` is not a valid index, then print the appropriate error and do not store anything in the history.
                print_invalid_index();
            }
            else
            {
                char *cmd = vector_get(history, command_line_nr);
                print_command(cmd);             // :warning: Print out the command **before executing** if there is a match.
                vector_push_back(history, cmd); // The command executed should be stored in the history.
                execute_command(cmd);           // Execute the command
            }
        }
        else if (buffer[0] == '!')
        {
            char *prefix = buffer + 1;
            int last_prefix_found = false;
            char *cmd = NULL;

            if (prefix == NULL)
            { // Print last cmd in history file, if exists
                if (!vector_empty(history))
                    {
                        cmd = *vector_back(history);
                        last_prefix_found = 1;
                    }
            }
            else
            {
                size_t prefix_len = strlen(prefix);
                size_t history_lines = vector_size(history);
                for (size_t i = history_lines - 1 ; i >= 0 && !last_prefix_found; --i)
                {
                        char *history_line = (char *) vector_at(history, i);
                        if (!strncmp(prefix, history_line, prefix_len)) {
                            cmd = history_line;
                            last_prefix_found = 1;
                        }
                }

            }

                if (!last_prefix_found)
                    print_no_history_match();
                else
                {
                    print_command(cmd);             // :warning: Print out the command **before executing** if there is a match.
                    vector_push_back(history, cmd); // The command executed should be stored in the history.
                    execute_command(cmd);           // Execute the command
                }
        }
        else
        { // All comamds that have to prompt to the `history` IN HERE
            vector_push_back(history, buffer);
            execute_command(buffer);
        }
    }

    // TODO: [Part 2] -> KILL all child processes when: a.) `exit` (notice break on strncmp exit) OR b.) EOF
    killAllChildProcesses();

    free(buffer); // used to read each line in the shell

    // Upon exiting, the shell should append the commands of the current session into the supplied history file
    if (h_flag)
    {
        FILE *output = fopen(history_path, "w");
        VECTOR_FOR_EACH(history, buffer, {
            fprintf(output, "%s\n", (char *)buffer);
        });

        fclose(output);
        free(history_path);
    }

    if (f_flag) // close file
        fclose(file);

    vector_destroy(history); // free vector
    return 0;
}
