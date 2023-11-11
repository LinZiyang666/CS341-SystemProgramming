/**
 * nonstop_networking
 * CS 341 - Fall 2023
 */
#include "common.h"
#include "format.h"
#include <errno.h>
#include <unistd.h>
#include <string.h>


ssize_t read_from_socket(int socket, char *buffer, size_t count) {
    // Your Code Here
    size_t bytes_read = 0;
    while (bytes_read < count)
    { // have not read all bytes
        int remaining_bytes_to_read = count - bytes_read;
        ssize_t newly_read_bytes = read(socket, buffer + bytes_read, remaining_bytes_to_read);
        if (newly_read_bytes == 0) // socket disconnected
            break;
        else if (newly_read_bytes == -1)
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

ssize_t write_to_socket(int socket, const char *buffer, size_t count)  {
    size_t bytes_wrote = 0;
    while (bytes_wrote < count)
    { // have not written all bytes
        int remaining_bytes_to_write = count - bytes_wrote;
        ssize_t newly_wrote_bytes = write(socket, (char *) buffer + bytes_wrote, remaining_bytes_to_write);
        if (newly_wrote_bytes == 0) // socket disconnected
            break;
        else if (newly_wrote_bytes == -1)
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

ssize_t read_header_from_socket(int socket, char *buffer, size_t count) {

    size_t bytes_read = 0;
    while (bytes_read < count) {

        ssize_t newly_read_bytes = read(socket, buffer + bytes_read, 1); // read one byte at a time, until `\n`

        if (newly_read_bytes == 0 || buffer[strlen(buffer) - 1] == '\n') // socket disconnected OR we finished reading the header
            break;
        else if (newly_read_bytes == -1) {
           // check error code
            if (errno == EINTR) // interrupted
                continue;       // retry
            else
                {
                    perror("read_header_from_socket failed");
                    return -1;
                }
        }

        bytes_read += newly_read_bytes;
    }

    return bytes_read;
}


int is_error(size_t read_bytes, size_t buffer_size) {
    if (read_bytes == 0 && read_bytes != buffer_size) // connection got closed
     {  
        print_connection_closed();
        return 1;
     }
     else if (read_bytes < buffer_size) {
        print_too_little_data();
        return 1;
     }
     else if (read_bytes > buffer_size) {
        print_received_too_much_data();
        return 1;
     }
     
     return 0; // No error :D
}

size_t get_min(size_t x, size_t y)
{
    if (x <= y)
        return x;
    else
        return y;
}