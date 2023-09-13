/**
 * shell
 * CS 341 - Fall 2023
 */
#include "format.h"
#include "shell.h"
#include "vector.h"
#include "sstring.h"
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

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
        else
        {
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

process *new_process(char *buffer, pid_t _pid)
{
    process *p = malloc(sizeof(process));
    p->pid = _pid;
    p->command = strdup(buffer); // TODO: free in destructor
    return p;
}

// This alternative does not work, as we still have to remove the process from the `processes` vector
// void delete_process (process **p_ptr) { // NOTE: passing the pointer by reference
//     process *p = *p_ptr;
//     if (p != NULL) {
//         free(p->command);
//         free(p);
//         p = NULL;
//     }
// }

void delete_process(pid_t _pid)
{
    size_t processes_len = vector_size(processes);
    size_t removed_pos = -1;

    for (size_t i = 0; i < processes_len; ++i)
    {
        process *p = vector_get(processes, i);
        if (p->pid == _pid)
        {
            free(p->command);
            free(p);
            p = NULL;
            removed_pos = i;
            break;
        }
    }

    vector_erase(processes, removed_pos);
}

// Return status code of executtion of command
// Here come all the commands that have to log in `history`
int execute_command(char *buffer)
{
    // `cd <path>` edge case: missing <path> arg
    if (!strcmp(buffer, "cd")) // cd
    {
        print_no_directory("");
        return 1;
    }
    // `cd <path>`
    else if(!strncmp(buffer, "cd ", 3))
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
    // For commands that are not built-in, the shell should consider the command name to be the name of a file that contains executable binary code
    else
    { // External commands must be executed by a new process, forked from your shell. If a command is not one of the built-in commands listed, it is an external command.

        // Tip: It is good practice to flush the standard output stream before the fork to be able to correctly display the output.
        // This will also prevent duplicate printing from the child process.
        fflush(stdout);
        pid_t child = fork();
        if (child == -1)
        {
            print_fork_failed();
            exit(1); // The child should exit with exit status 1 if it fails to execute a command.
        }
        else if (child == 0)
        { // I am child

            // Get all C-strings (char *s) to pass as arguments to `execvp(...)`
            // char *[] to statically allocate on stack
            sstring *cmds_sstring = cstr_to_sstring(buffer); // NOTE: don't have to free if `exec` succesfully executes, as the swapped image will clear stack/heap
            vector *cmds_vector = sstring_split(cmds_sstring, ' ');
            size_t cmds_len = vector_size(cmds_vector);
            char *cmds[cmds_len + 1];

            for (size_t i = 0; i < cmds_len; ++i)
            {
                char *cmd = vector_get(cmds_vector, i);
                cmds[i] = cmd;
            }
            cmds[cmds_len] = NULL; // NULL terminate the array to prepare for `execvp`

            print_command_executed(getpid());
            int succ_exec = execvp(cmds[0], cmds);
            if (succ_exec == -1) // exec only returns to the child process when the command fails to execute successfully
            {
                print_exec_failed(cmds[0]);
                exit(1);
            }
        }
        else
        { // I am parent
            // TODO: You are responsible of cleaning up all the child processes upon termination of your program

            process *p = new_process(buffer, child);
            vector_push_back(processes, p);

            // TODO: [PART 2] background processes: buffer[sz - 1] == '&'

            // Foreground process: setpgid(child, parent)
            int succesfully_set_pgid = setpgid(child, getpid());
            if (succesfully_set_pgid == -1)
            {
                print_setpgid_failed();
                exit(1);
            }

            // The parent is still responsible for waiting on the child. Avoid creating Zombies
            int status;

            if (waitpid(child, &status, 0) == -1)
            {
                print_wait_failed();
                exit(1);
            }
            else
            {
                delete_process(child);                             // Destroy child process until termination of program
                if (WIFEXITED(status) && WEXITSTATUS(status) != 0) // Child failed
                    return 1;                                      // Prepare for &&, || and ;
            }
        }
    }

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
        if (strlen(buffer) == 0) // Empty string
            continue;
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
            int last_prefix_found = 0;
            char *cmd = NULL;

            if (buffer[1] == '\0') // prefix may be empty
            {                      // Print last cmd in history file, if exists
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
                for (int i = history_lines - 1; i >= 0 && !last_prefix_found; i--)
                {
                    char *history_line = (char *)vector_get(history, i);
                    if (!strncmp(prefix, history_line, prefix_len))
                    {
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
        { // Logical Ops., `cd` OR external commands
            vector_push_back(history, buffer);
            int logical = 0;

            sstring *cmds_sstring = cstr_to_sstring(buffer);        // We'll have free it afterward
            vector *cmds_vector = sstring_split(cmds_sstring, ' '); //
            size_t cmds_len = vector_size(cmds_vector);

            for (size_t i = 0; i < cmds_len && !logical; ++i)
            {
                char *cmd = vector_get(cmds_vector, i);
                char *left;
                char *right;

                if (!strcmp(cmd, "&&"))
                {
                    right = strdup(buffer);
                    char *free_ptr = right;
                    left = strsep(&right, "&");
                    left[strlen(left) - 1] = '\0'; // Remove trailing space
                    right += 2;
                    int failed_cmd1 = execute_command(left);
                    if (!failed_cmd1)
                        execute_command(right);
                    free(free_ptr);
                }
                else if (!strcmp(cmd, "||"))
                {
                    right = strdup(buffer);
                    char *free_ptr = right;
                    left = strsep(&right, "|");
                    left[strlen(left) - 1] = '\0'; // Remove trailing space
                    right += 2;
                    int failed_cmd1 = execute_command(left);
                    if (failed_cmd1)
                        execute_command(right);
                    free(free_ptr);
                }
                else if (cmd[strlen(cmd) - 1] == ';')
                {
                    right = strdup(buffer);
                    char *free_ptr = right;
                    left = strsep(&right, ";");
                    right += 1;
                    execute_command(left);
                    execute_command(right);
                    free(free_ptr);
                }

                if (!strcmp(cmd, "&&") || !strcmp(cmd, "||") || cmd[strlen(cmd) - 1] == ';')
                    logical = 1;
            }

            sstring_destroy(cmds_sstring);
            vector_destroy(cmds_vector);

            if (!logical)
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
