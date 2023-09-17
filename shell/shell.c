/**
 * shell
 * CS 341 - Fall 2023
 */
#include "shell.h"
#include "format.h"
#include "sstring.h"
#include "vector.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>

typedef struct process
{
  char *command;
  pid_t pid;
} process;

static vector *processes;
static int h_flag = 0;
static int f_flag = 0;
static FILE *file =
    NULL; // Our input stream, may be `stdin` or a file if `-f` is specified

static char *history_path = NULL;

void print_full_path()
{
  // Print before reading a new line
  char *cwd = get_full_path("./");
  print_prompt(cwd,
               getpid()); // When prompting for a command, the shell will print
                          // a prompt in the following format (from format.h):
  free(cwd);              // Ensures any flow control gets here
}

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
  { // When provided `-h`, the shell should load in the history file
    // as its history.
    FILE *history_file;

    history_path =
        get_full_path(h_name);               // TODO: free it (when writing back to history)
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
    // When provided -f, your shell will both print and run the commands in the
    // file in sequential order until the end of the file
    file = fopen(f_name, "r");
    if (!file)
    { // non-existent script file
      print_script_file_error();
      exit(1);
    }
  }
  else // read from `stdin`
    file = stdin;

  print_full_path();
}

// Remove process with PID = <pid>, by freeing it from the memory & removing it
// from the `processes` vector
void remove_process(pid_t pid)
{
  for (size_t i = 0; i < vector_size(processes); ++i)
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

// Source -
// https://stackoverflow.com/questions/11322488/how-to-make-sure-that-waitpid-1-stat-wnohang-collect-all-children-process
void wait_for_all_background_child_processes();

void handle_signals()
{
  signal(SIGINT, handle_SIGINT);
  signal(SIGCHLD, wait_for_all_background_child_processes);
}

process *new_process(char *buffer, pid_t _pid)
{
  process *p = malloc(sizeof(process));
  p->pid = _pid;
  p->command = strdup(buffer); // TODO: free in destructor
  return p;
}

// This alternative does not work, as we still have to remove the process from
// the `processes` vector void delete_process (process **p_ptr) { // NOTE:
// passing the pointer by reference
//     process *p = *p_ptr;
//     if (p != NULL) {
//         free(p->command);
//         free(p);
//         p = NULL;
//     }
// }

void delete_process(pid_t _pid)
{
  size_t removed_pos = -1;

  for (size_t i = 0; i < vector_size(processes); ++i)
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

void wait_for_all_background_child_processes()
{
  pid_t pid;
  while ((pid = waitpid(-1, 0, WNOHANG)) > 0){ 
     
  };
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
  else if (!strncmp(buffer, "cd ", 3))
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
  // For commands that are not built-in, the shell should consider the command
  // name to be the name of a file that contains executable binary code
  else
  { // External commands must be executed by a new process, forked from
    // your shell. If a command is not one of the built-in commands listed,
    // it is an external command.

    // Tip: It is good practice to flush the standard output stream before the
    // fork to be able to correctly display the output. This will also prevent
    // duplicate printing from the child process.
    fflush(stdout);
    pid_t child = fork();
    if (child == -1)
    {
      print_fork_failed();
      exit(1); // The child should exit with exit status 1 if it fails to
               // execute a command.
    }
    else if (child == 0)
    { // I am child

      // Get all C-strings (char *s) to pass as arguments to `execvp(...)`
      // char *[] to statically allocate on stack
      sstring *cmds_sstring = cstr_to_sstring(
          buffer); // NOTE: don't have to free if `exec` succesfully executes,
                   // as the swapped image will clear stack/heap
      vector *cmds_vector = sstring_split(cmds_sstring, ' ');
      size_t cmds_len = vector_size(cmds_vector);
      char *cmds[cmds_len + 1];

      for (size_t i = 0; i < cmds_len; ++i)
      {
        char *cmd = vector_get(cmds_vector, i);
        cmds[i] = cmd;
      }
      if (cmds_len > 0 && !strcmp(cmds[cmds_len - 1], "&")) // background cmd
        cmds[cmds_len - 1] = NULL;
      else
        cmds[cmds_len] =
            NULL; // NULL terminate the array to prepare for `execvp`

      print_command_executed(getpid());
      int succ_exec = execvp(cmds[0], cmds);
      if (succ_exec == -1) // exec only returns to the child process when the
                           // command fails to execute successfully
      {
        print_exec_failed(cmds[0]);
        exit(1);
      }
    }
    else
    { // I am parent
      // TODO: You are responsible of cleaning up all the child processes upon
      // termination of your program

      process *p = new_process(buffer, child);
      vector_push_back(processes, p);

      size_t buffer_len = strlen(buffer);
      if (buffer_len > 0 &&
          buffer[buffer_len - 1] == '&')
      { // Background process
        int succesfully_set_pgid = setpgid(child, child);
        if (succesfully_set_pgid == -1)
        {
          print_setpgid_failed();
          exit(1);
        }
        // Don't block (as we would have done in Foreground processe) -> "the
        // shell should be ready to take the next command before the given
        // command has finished running" All the children will be waited when
        // SIGCHLD
      }
      else
      {
        // Foreground process: setpgid(child, parent)
        int succesfully_set_pgid = setpgid(child, getpid());
        if (succesfully_set_pgid == -1)
        {
          print_setpgid_failed();
          exit(1);
        }

        // The parent is still responsible for waiting on the child. Avoid
        // creating Zombies
        int status;

        if (waitpid(child, &status, 0) == -1)
        {
          print_wait_failed();
          exit(1);
        }
        else
        {
          delete_process(
              child);                                        // Destroy child process until termination of program
          if (WIFEXITED(status) && WEXITSTATUS(status) != 0) // Child failed
            return 1;                                        // Prepare for &&, || and ;
        }
      }
      
    }
  }

  return 0;
}

void free_process(process *p)
{
  free(p->command);
  free(p);
  p = NULL;
}

void destroy_proc_info(process_info *p_info)
{
  free(p_info->start_str);
  free(p_info->time_str);
  free(p_info->command);
  free(p_info);
  p_info = NULL;
}

// Source: How to extract info from /proc in Linux C:
// https://stackoverflow.com/questions/33266678/how-to-extract-information-from-the-content-of-proc-files-on-linux-using-c
//
// long int process_proc_file() {
//
// }

// Create a process_info* entry from a process*, by making use of /proc
process_info *build_proc_info(process *p)
{

  process_info *p_info = malloc(sizeof(process_info));

  char filename[1000];
  sprintf(filename, "/proc/%d/stat", p->pid);
  FILE *f = fopen(filename, "r");
  if (!f)
  {
    print_script_file_error();
    exit(1);
  }
  char buffer[1000];
  memset(buffer, 0, sizeof(buffer));
  fgets(buffer, 1000, f);
  sstring *buffer_sstr = cstr_to_sstring(buffer);
  vector *stat_fields = sstring_split(buffer_sstr, ' ');

  p_info->pid = p->pid;
  int expected_pid = atoi((char *)vector_get(stat_fields, 0));
  assert(p_info->pid == expected_pid);

  char *expected_nthreads_str = vector_get(stat_fields, 19);
  p_info->nthreads = atol(expected_nthreads_str);
  p_info->vsize = (unsigned long)atol(vector_get(stat_fields, 22));
  char *expected_state_str = (char *)vector_get(stat_fields, 2);
  p_info->state = (char)(expected_state_str[0]);

  p_info->start_str = malloc(100); // long takes at most 8 bytes
  time_t total_seconds_start = atol((char *)vector_get(stat_fields, 21)) / sysconf(_SC_CLK_TCK);
  struct tm *local_time = localtime(&total_seconds_start);

  size_t nbytes_start_str = time_struct_to_string(p_info->start_str, 100, local_time);
  if (nbytes_start_str >= 100)
    exit(1);

  // As mentioned in the man pages of proc/procfs -> divide by sysconf(_SC_CLK_TCK) to get time measured in seconds
  long utime = atol((char *)vector_get(stat_fields, 13)) / sysconf(_SC_CLK_TCK);
  long stime = atol((char *)vector_get(stat_fields, 14)) / sysconf(_SC_CLK_TCK);

  char cpu_str[100];
  int nbytes_cpu = execution_time_to_string(cpu_str, 100, (utime + stime) / 60, (utime + stime) % 60);

  if (nbytes_cpu >= 100)
    exit(1);

  p_info->time_str = strdup(cpu_str);

  p_info->command = strdup(p->command);

  sstring_destroy(buffer_sstr);
  vector_destroy(stat_fields);
  fclose(f);
  return p_info;
}

//
// Entire logic of when `ps` built-in is called
void execute_ps()
{ // Make use of `processes` process* vector to extract each
  // process*, and then compute its process info from its
  // associated process* entry
  print_process_info_header();
  size_t processes_len = vector_size(processes);
  printf("%ld\n", processes_len);

  for (size_t i = 0; i < processes_len; ++i)
  {
    process *p = (process *)vector_get(processes, i);
    process_info *p_info = build_proc_info(p);
    print_process_info(p_info);
    destroy_proc_info(p_info);
  }

  //  For the main shell process only, you do not need to include the
  //  command-line flags
  process *main_p = new_process(
      "./shell", getpid()); // TODO: free it, without removing from vector
  process_info *p_info = build_proc_info(main_p);
  print_process_info(p_info);
  destroy_proc_info(p_info);

  free_process(main_p);
}


process *get_process_by_pid(pid_t pid) {
  size_t processes_len = vector_size(processes);
  for (size_t i = 0; i < processes_len; ++i) {
    process *p = (process *) vector_get(processes,i);
    if (p -> pid == pid) 
      return p;
  }

  return NULL; // If process w/ <pid> not found
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

    // Keep track of commands in history; also - prepare buffer to print: Note
    // the lack of a newline at the end of this prompt.

    if (nread > 0 && buffer[nread - 1] == '\n')
    {
      buffer[nread - 1] = '\0';
      // Your shell should also support running a series of commands from a
      // script f
      if (file !=
          stdin) // As `stdin` already gets displayed to the shell session, we
                 // want to display to shell only if '-f/-h' is used to read
        print_command(buffer);
    }

    // TODO: [PART 1] solve built-in commands - Run in shell (main / parent)
    // process, don't `fork()`
    if (strlen(buffer) == 0) // Empty string
    {
      print_full_path();
      continue;
    }
    if (!strncmp(buffer, "exit", 4))
      break;
    else if (!strcmp(buffer, "!history"))
    { // !history
      size_t history_lines =
          vector_size(history); // Print `history` vector line by line
      for (size_t i = 0; i < history_lines; ++i)
        print_history_line(i, (char *)vector_get(history, i));
    }
    else if (buffer[0] == '#')
    { // '#<n>
      size_t command_line_nr = atoi(
          buffer +
          1); // Precondition: "buffer + 1" represents a valid integer: n >= 0
      if (command_line_nr >=
          vector_size(history))
      { //  If `n` is not a valid index, then print
        //  the appropriate error and do not store
        //  anything in the history.
        print_invalid_index();
      }
      else
      {
        char *cmd = vector_get(history, command_line_nr);
        print_command(cmd); // :warning: Print out the command **before
                            // executing** if there is a match.
        vector_push_back(
            history,
            cmd);             // The command executed should be stored in the history.
        execute_command(cmd); // Execute the command
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
        print_command(cmd); // :warning: Print out the command **before
                            // executing** if there is a match.
        vector_push_back(
            history,
            cmd);             // The command executed should be stored in the history.
        execute_command(cmd); // Execute the command
      }
    } // TODO: built-in PART2
    else if (!strcmp(buffer,
                     "ps"))
    { //`ps` is a built-in command for your shell
      execute_ps();
    }
    else if (!strncmp(buffer, "kill", 4)) {
      if (strlen(buffer) == 4) // kill was run without a pid
        print_invalid_command(buffer);
      else 
      {
        pid_t pid;
        sscanf(buffer + 4, "%d", &pid);
        process *p = get_process_by_pid(pid);
        if (!p) 
          print_no_process_found(pid);
        else {
          kill(pid, SIGKILL); // Kill the process
          print_killed_process(p ->pid, p->command);
          remove_process(pid); // Remove process from vector & memory (free it)
        }
      }
    }
    else if (!strncmp(buffer, "stop", 4)) {
      if (strlen(buffer) == 4) // stop was run without a pid
        print_invalid_command(buffer);
      else
      {
        pid_t pid;
        sscanf(buffer + 4, "%d", &pid);
        process *p = get_process_by_pid(pid);
        if (!p) 
          print_no_process_found(pid);
        else {
          kill(pid, SIGSTOP); // Kill the process
          print_stopped_process(p ->pid, p->command);
        }
      }
    }
    else if (!strncmp(buffer, "cont", 4)) {
      if (strlen(buffer) == 4) // cont was run without a pid
        print_invalid_command(buffer);
      else
      {
        pid_t pid;
        sscanf(buffer + 4, "%d", &pid);
        process *p = get_process_by_pid(pid);
        if (!p) 
          print_no_process_found(pid);
        else {
          kill(pid, SIGCONT); // Kill the process
          print_continued_process(p -> pid, p -> command);
        }
      }

    }
    else
    { // Logical Ops., `cd` OR external commands
      vector_push_back(history, buffer);
      int logical = 0;

      sstring *cmds_sstring =
          cstr_to_sstring(buffer);                            // We'll have free it afterward
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

        if (!strcmp(cmd, "&&") || !strcmp(cmd, "||") ||
            cmd[strlen(cmd) - 1] == ';')
          logical = 1;
      }

      sstring_destroy(cmds_sstring);
      vector_destroy(cmds_vector);

      if (!logical)
        execute_command(buffer);
    }
    print_full_path();
  }

  // TODO: [Part 2] -> KILL all child processes when: a.) `exit` (notice break
  // on strncmp exit) OR b.) EOF
  killAllChildProcesses();

  free(buffer); // used to read each line in the shell

  // Upon exiting, the shell should append the commands of the current session
  // into the supplied history file
  if (h_flag)
  {
    FILE *output = fopen(history_path, "w");
    VECTOR_FOR_EACH(history, buffer,
                    { fprintf(output, "%s\n", (char *)buffer); });

    fclose(output);
    free(history_path);
  }

  if (f_flag) // close file
    fclose(file);

  vector_destroy(history); // free vector
  return 0;
}
