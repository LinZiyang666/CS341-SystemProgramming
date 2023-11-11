/**
 * nonstop_networking
 * CS 341 - Fall 2023
 */
#pragma once
#include <stddef.h>
#include <sys/types.h>

#define MAX_HEADER_LEN 1024
#define MAX_FILENAME 256
#define MAX_ERROR_LEN 32 

#define MAX_CLIENTS 10 
#define MAX_EVENTS 100 

// 5000ms = 5s
#define MAX_TIMEOUT 5000

static char *OK = "OK\n";
static char *ERROR = "ERROR\n";

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

// Read `count` ytes from `socket` into `buffer`
// This method is invoked for reading the header of a request received from the client, on the server side
// HEADER_MAX_LENGTH = 1024; reads byte by byte as we don't now how long the header will be
ssize_t read_header_from_socket(int socket, char *buffer, size_t count);

// RET: 1, if an error is detected (and calls `print_connection_closed`, `print_too_little_data`, or `print_received_too_much_data`), depending on the error
// or 0, if no error was found
int is_error(size_t sz1, size_t sz2);

// std::min
size_t get_min(size_t x, size_t y);