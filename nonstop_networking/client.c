/**
 * nonstop_networking
 * CS 341 - Fall 2023
 */
#include "format.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


static char **args;
static int server_fd; // socket exposed by server

char **parse_args(int argc, char **argv);
verb check_args(char **args);

// Establish TCP connection from client to server
// RET: server_fd (file descriptor that describes the server socket) on success, -1 on error
int connect_to_server(char *host, char *port);
int run_client_request(verb req);
int read_server_response(verb req);
void cleanup_resources();

int main(int argc, char **argv)
{
    // Good luck!
    // parse args
    args = parse_args(argc, argv);
    // check args
    verb req = check_args(args);
    if (req == V_UNKNOWN)
    {
        exit(1);
    }

    char *host = argv[0];
    char *port = argv[1];
    server_fd = connect_to_server(host, port);
    if (-1 == server_fd)
    { // unable to connect to the erver
        exit(1);
    }

    if (-1 == run_client_request(req)) // unable to run client req
    {
        exit(1);
    }

    if (-1 == read_server_response(req))
    {
        exit(1);
    }

    cleanup_resources();
    return 0;
}

// ---------------- HELPER METHODS ----------------

int connect_to_server(char *host, char *port)
{

    struct addrinfo hints, *res;
    memset((void*) &hints, 0, sizeof(hints));

    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP

    int gai = getaddrinfo(host, port, &hints, &res); // obtain the address info of the server
    if (gai) { // err.
        fprintf(stderr, "getaddrinfo failed w/: %s", gai_strerror(gai));
        return -1;
    }

    int sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (-1 == sock_fd) {
        perror("Client's socket failed");
        return -1;
    }

    int conn = connect(sock_fd, res->ai_addr, res->ai_addrlen);
    if (-1 == conn) {
        perorr("connect failed");
       return -1;
    }

    freeaddrinfo(res);
    return sock_fd;
}

void cleanup_resources()
{
    shutdown(server_fd, SHUT_RD); // we are done listening to the server socket
    close(server_fd);             // close server socket
    free(args);                   // args were calloc'd in `parse_args`
    args = NULL;                  // ensure use-after-free safety
}

/**
 * Given commandline argc and argv, parses argv.
 *
 * argc argc from main()
 * argv argv from main()
 *
 * Returns char* array in form of {host, port, method, remote, local, NULL}
 * where `method` is ALL CAPS
 */
char **parse_args(int argc, char **argv)
{
    if (argc < 3)
    {
        return NULL;
    }

    char *host = strtok(argv[1], ":");
    char *port = strtok(NULL, ":");
    if (port == NULL)
    {
        return NULL;
    }

    char **args = calloc(1, 6 * sizeof(char *));
    args[0] = host;
    args[1] = port;
    args[2] = argv[2];
    char *temp = args[2];
    while (*temp)
    {
        *temp = toupper((unsigned char)*temp);
        temp++;
    }
    if (argc > 3)
    {
        args[3] = argv[3];
    }
    if (argc > 4)
    {
        args[4] = argv[4];
    }

    return args;
}

/**
 * Validates args to program.  If `args` are not valid, help information for the
 * program is printed.
 *
 * args     arguments to parse
 *
 * Returns a verb which corresponds to the request method
 */
verb check_args(char **args)
{
    if (args == NULL)
    {
        print_client_usage();
        exit(1);
    }

    char *command = args[2];

    if (strcmp(command, "LIST") == 0)
    {
        return LIST;
    }

    if (strcmp(command, "GET") == 0)
    {
        if (args[3] != NULL && args[4] != NULL)
        {
            return GET;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "DELETE") == 0)
    {
        if (args[3] != NULL)
        {
            return DELETE;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "PUT") == 0)
    {
        if (args[3] == NULL || args[4] == NULL)
        {
            print_client_help();
            exit(1);
        }
        return PUT;
    }

    // Not a valid Method
    print_client_help();
    exit(1);
}
