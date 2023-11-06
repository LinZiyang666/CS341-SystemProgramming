/**
 * nonstop_networking
 * CS 341 - Fall 2023
 */
#pragma once
#include <stddef.h>
#include <sys/types.h>

#define MAX_HEADER_LEN 1024
#define MAX_FILENAME 256

#define LOG(...)                      \
    do {                              \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n");        \
    } while (0);

typedef enum { GET, PUT, DELETE, LIST, V_UNKNOWN } verb;


// `read_from_socket` and `write_to_socket` taken from `charming_chatroom` lab assignment impl.
// used by the client to write to the socket the server exposed (send data client -> server)

// Reads `count` bytes from `socket` into the `buffer` 
// RET: `bytes_read`: the number of bytes succesfully read from the buffer, or -1 in case of an error
ssize_t read_from_socket(int socket, char *buffer, size_t count);

// Write `count` bytes from `buffer` into the `socket` 
// RET: `wrote_bytes`: the number of bytes succesfully wrote into the buffer, or -1 in case of an error
ssize_t write_to_socket(int socket, const char *buffer, size_t count);

// RET: 1, if an error is detected (and calls `print_connection_closed`, `print_too_little_data`, or `print_received_too_much_data`), depending on the error
// or 0, if no error was found
int is_error(size_t sz1, size_t sz2);