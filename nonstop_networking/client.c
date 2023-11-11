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
#include <sys/stat.h>
#include <netdb.h>
#include <errno.h>

static char **parsed_args; // NOTE: make sure to use these instead of argv[] afetr `parse_args`
static int server_fd;      // socket exposed by server

char **parse_args(int argc, char **argv);
verb check_args(char **args);

// Establish TCP connection from client to server
// RET: server_fd (file descriptor that describes the server socket) on success, -1 on error
int connect_to_server(char *host, char *port);

// Construct the client request and write it to the socket, so that the SERVER can receive it.
// If PUT: to write size and binary data to server
// Once the client has sent all the data to the server, it should perform a ‘half close’ by closing the write half of the socket (hint: shutdown()).
// This ensures that the server will eventually realize that the client has stopped sending data, and can move forward with processing the request
// RET: 0 if succesfull, -1 if error
int run_client_request(verb req);

// Handles the PUT case - transfer # of bytes server should expect & actual data
// Reads from local indicated file, and write data to a buffer 1024 by 1024 bytes (or rem_bytes if rem_bytes < 1024); and then writes to the socket of the server
int perform_put();


int read_server_response(verb req);
void cleanup_resources();

int main(int argc, char **argv)
{
    // Good luck!
    // parse args
    parsed_args = parse_args(argc, argv);
    // check args
    verb req = check_args(parsed_args);
    if (req == V_UNKNOWN)
    {
        exit(1);
    }

    char *host = parsed_args[0];
    char *port = parsed_args[1];
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
    memset((void *)&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP

    int gai = getaddrinfo(host, port, &hints, &res); // obtain the address info of the server
    if (gai)
    { // err.
        fprintf(stderr, "getaddrinfo failed w/: %s", gai_strerror(gai));
        return -1;
    }

    int sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (-1 == sock_fd)
    {
        perror("Client's socket failed");
        return -1;
    }

    int conn = connect(sock_fd, res->ai_addr, res->ai_addrlen);
    if (-1 == conn)
    {
        perror("connect failed");
        return -1;
    }

    freeaddrinfo(res);
    // fprintf(stderr, "Succesfully connected");
    return sock_fd;
}

int run_client_request(verb req)
{
    char *client_request = NULL;
    char *cmd = parsed_args[2];
    char *remote = parsed_args[3];

    if (req == LIST)
    {                                                     // "LIST\n"
        client_request = calloc(1, strlen("LIST\n") + 1); // +1 for '\0'
        sprintf(client_request, "%s\n", cmd);
    }
    else
    {                                                                 // "GET [remote]\n", "PUT [remote]\n" or "DELETE [remote]\n"
        client_request = calloc(1, strlen(cmd) + strlen(remote) + 3); // +1 for ' ', +1 for '\n' and +1 for '\0'
        sprintf(client_request, "%s %s\n", cmd, remote);
    }

    size_t bytes_to_write = strlen(client_request);
    size_t bytes_wrote = write_to_socket(server_fd, client_request, bytes_to_write);

    if (bytes_wrote < bytes_to_write)
    { // Connection got closed while we were writting our data; encapsulates the case when bytes_wrote == -1
        print_connection_closed();
        return -1;
    }

    if (client_request)
        free(client_request);

    // if PUT: write # of bytes the server should expecte to receive & binary data that follows
    if (req == PUT)
    {
        int put_err = perform_put();
        if (-1 == put_err)
            return -1;
    }
    return 0;
}

int perform_put()
{
    char *local = parsed_args[4]; // path to local file
    struct stat stat_local;

    int stat_err = stat(local, &stat_local);
    if (-1 == stat_err)
        return -1;

    size_t local_size = stat_local.st_size;
    // Step1: write # of bytes the server should expect to receive; sizeof(size_t) = 8
    // Interpret the `off_t local_size` as a `char*`, so that the `write_to_socket` method can write byte by byte from this integer
    // effectively saying, "Here's where you can start reading bytes to send over the socket."
    write_to_socket(server_fd, (char *)&local_size, sizeof(size_t));

    // Step2: Write binary data

    FILE *local_file = fopen(local, "r");
    if (!local_file) // On PUT, if local file does not exist, simply exiting is okay. -> will exit in `main`
        return -1;

    size_t bytes_wrote = 0;
    while (bytes_wrote < local_size)
    {
        size_t new_bytes_wrote = get_min(local_size - bytes_wrote, (size_t)MAX_HEADER_LEN); // ensure we don't wirte more than `local_size` bytes, while keeping writing bytes 1024 by 1024
        // should maintain some (reasonably) fixed size buffers (say, 1024 bytes), and reuse these buffers as you send or receive data over time.
        char buffer[MAX_HEADER_LEN + 1];

        // Read from `local_file` into `buffer`
        fread(buffer, 1, new_bytes_wrote, local_file);
        //Write from `buffer` to socket
        size_t bytes_wrote_to_socket = write_to_socket(server_fd, buffer, new_bytes_wrote);
        if (bytes_wrote_to_socket < new_bytes_wrote) // connection got closed in the meantime
        {
            print_connection_closed();
            return -1;
        }

        bytes_wrote += new_bytes_wrote;
    }

    fclose(local_file);
    // Once the client has sent all the data to the server, it should perform a ‘half close’ by closing the write half of the socket (hint: shutdown()).
    int shutdown_err = shutdown(server_fd, SHUT_WR);
    if (-1 == shutdown_err) {
        perror("Failed to shutdown() Write part");
        return -1;
    }

    return 0;
}

int read_server_response(verb req)
{
    // TODO: check correctness & discuss at LAB
    char *response = calloc(1, strlen(OK) + 1);
    size_t status_bytes_read = read_from_socket(server_fd, response, strlen(OK));  // We will use this variable to read the remaining `strlen(ERROR) - status_bytes_read` bytes in the case of ERROR

    if (strcmp(response, OK) == 0) { // Succesful request returned from the server
        fprintf(stdout, "%s\n", OK); // TODO: check if needed

        if (req == PUT || req == DELETE)
            print_success();
        if (req == LIST) { // For LIST, binary data from the server should be printed to STDOUT, each file on a separate line. 

            size_t size = 0;
            read_from_socket(server_fd, (char*)&size, sizeof(size_t)); // read the size that we expect to receive, that we will have to read later on
            char buffer[size + 6]; 
            memset(buffer, 0, size + 6);
            status_bytes_read = read_from_socket(server_fd, buffer, size + 5);
            if (is_error(status_bytes_read, size)) {
                free(response);
                return -1;
            }
            else {
                fprintf(stdout, "%zu%s", size, buffer); // TODO: check if correct format
                fflush(stdout);
            }
        }

        if (req == GET) { 
            // For GET, binary data should be written to the [local] file specified when the user ran the command. 
            // If not created, create the file. If it exists, truncate the file. You should create the file with all permissions set (r-w-x for all users groups).
            // Read from socket, write to `local_file`
            char *local = parsed_args[4]; // path to local file
            FILE *local_file = fopen(local, "w+"); // TODO: check if flag is correct
            if (!local_file) {
                perror("fopen() failed in GET OK");
                free(response);
                return -1; 
            }

            if (chmod(local, 0777) == -1) // You should create the file with all permissions set (r-w-x for all users groups). 
            {
                perror("chmod");
                free(response);
                return -1;
            }


            size_t size = 0;
            read_from_socket(server_fd, (char *)&size, sizeof(size_t));
            size_t bytes_read = 0;
            // \r\n\r\n -> 4 bytes
            // HTTP Web requests:  \r\n forms the standard line terminator in HTTP protocol

            while (bytes_read < size + 5) { // TODO: check -> https://stackoverflow.com/questions/6686261/what-at-the-bare-minimum-is-required-for-an-http-request
                size_t new_bytes_read = get_min(size + 5 - bytes_read, MAX_HEADER_LEN); 
                char buffer[MAX_HEADER_LEN + 1] = {0}; // ChatGPT: , if you're simply initializing a freshly declared array, the direct initializer is more concise and idiomatic C.
                size_t bytes_read_from_socket = read_from_socket(server_fd, buffer, new_bytes_read); // read from socket

                if (bytes_read_from_socket == 0) // finished reading | connection got closed
                    break;

                fwrite(buffer, 1, bytes_read_from_socket, local_file); // write to `local_file`

                bytes_read += bytes_read_from_socket;
            }

            if (is_error(bytes_read, size)) {
                free(response);
                return -1;
                }

            fclose(local_file);
        }

    }
    else { // handle ERROR from server
        char *err_response = calloc(1, strlen(ERROR) + 1);
        strcpy(err_response, response);
        free(response); 

        // fprintf(stderr, "Before %s\n", err_response);
        read_from_socket(server_fd, err_response + status_bytes_read, strlen(ERROR) - status_bytes_read);
        // fprintf(stderr, "After %s\n", err_response);
        if (strcmp(err_response, ERROR) == 0) {
            fprintf(stdout, "%s\n", ERROR); // TODO: check if needed
            char error_msg[MAX_ERROR_LEN + 1] = {0};
            int read_error = read_from_socket(server_fd, error_msg, MAX_ERROR_LEN);
            if (0 == read_error) {   
                print_connection_closed();
            }
            print_error_message(error_msg);
        }
        else {
            print_invalid_response();
        }

        free(err_response);
        return 0;
    }

    free(response);
    return 0;
}

void cleanup_resources()
{
    shutdown(server_fd, SHUT_RD); // we are done listening to the server socket
    close(server_fd);             // close server socket
    free(parsed_args);            // `parsed_args` were calloc'd in `parse_args`
    parsed_args = NULL;           // ensure use-after-free safety
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
