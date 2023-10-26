/**
 * charming_chatroom
 * CS 341 - Fall 2023
 */
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "utils.h"
static const size_t MESSAGE_SIZE_DIGITS = 4;

char *create_message(char *name, char *message)
{
    int name_len = strlen(name);
    int msg_len = strlen(message);
    char *msg = calloc(1, msg_len + name_len + 4);
    sprintf(msg, "%s: %s", name, message);

    return msg;
}

ssize_t get_message_size(int socket)
{
    // Network to host
    int32_t size;
    ssize_t read_bytes =
        read_all_from_socket(socket, (char *)&size, MESSAGE_SIZE_DIGITS);
    if (read_bytes == 0 || read_bytes == -1)
        return read_bytes;

    return (ssize_t)ntohl(size);
}

// You may assume size won't be larger than a 4 byte integer
ssize_t write_message_size(size_t size, int socket)
{
    // Your code here
    // Host to network

    size_t network_size = htonl(size);

    ssize_t wrote_bytes =
        write_all_to_socket(socket, (char *)&network_size, MESSAGE_SIZE_DIGITS);

    if (wrote_bytes == 0 || wrote_bytes == -1)
        return wrote_bytes;

    return wrote_bytes;
}

ssize_t read_all_from_socket(int socket, char *buffer, size_t count)
{
    // Your Code Here
    ssize_t bytes_read = 0;
    while (bytes_read < count)
    { // have not read all bytes
        int remaining_bytes_to_read = count - bytes_read;
        ssize_t newly_read_bytes = read(socket, buffer + bytes_read, remaining_bytes_to_read);
        if (newly_read_bytes == -1)
        { // error
            // check error code
            if (errno == EINTR) // interrupted
                continue;       // retry
            else
                return -1; // actual error
        }

        bytes_read += newly_read_bytes; 
    }
    return bytes_read;
}

ssize_t write_all_to_socket(int socket, const char *buffer, size_t count)
{
    ssize_t bytes_wrote = 0;
    while (bytes_wrote < count)
    { // have not written all bytes
        int remaining_bytes_to_write = count - bytes_wrote;
        ssize_t newly_wrote_bytes = read(socket, buffer + bytes_wrote, remaining_bytes_to_write);
        if (newly_wrote_bytes == -1)
        { // error
            // check error code
            if (errno == EINTR) // interrupted
                continue;       // retry
            else
                return -1; // actual error
        }

        bytes_wrote += newly_wrote_bytes;
    }
    return bytes_wrote;
}
