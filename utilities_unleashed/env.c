/**
 * utilities_unleashed
 * CS 341 - Fall 2023
 */

#include <string.h>
#include "format.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{

    if (argc < 3)
        print_env_usage();

    pid_t child = fork();
    if (child == -1)
        print_fork_failed();
    else if (child > 0)
    { // parent
        int status;
        waitpid(child, &status, 0);
        return 0;
    }
    else
    { // child

        int i = 1;
        for (; i < argc; ++i)
        {

            char *arg = argv[i];

            if (strchr(arg, '=')) // key-value pair
            {
                char *val = arg;
                char *key = strsep(&val, "=");

                if (strlen(val) > 0 && val[0] == '%') // reference
                {
                    val++;
                    char *match = getenv(val);
                    if (match) 
                        val = match;
                    else
                        val = ""; // Variable not found - this is the case when `getenv` fails
                }
                else if (strlen(val) == 0) // "NAME="  => invalid input
                    print_env_usage();

                int contains_equals_sign = setenv(key, val, 1);
                if (contains_equals_sign) print_environment_change_failed();
            }
            else
            {
                if (!strcmp(arg, "--")) // the next arg (argv[i + 1]) is the command; argv[i + i + 2 ... argc - 1] are the args of cmd.
                {
                    if (i + 1 >= argc || (i + 1 < argc && argv[i + 1] == NULL)) // Cannot find cmd after --
                         print_env_usage();

                    execvp(*(argv + i + 1), argv + i + 1);
                    print_exec_failed();
                }
                else // Cannot find -- in arguments
                    print_env_usage(); // Also takes into account: Cannot find = in an variable argument
            }
        }
    }

    return 0;
}
