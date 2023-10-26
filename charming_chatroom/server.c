/**
 * charming_chatroom
 * CS 341 - Fall 2023
 */
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

#define MAX_CLIENTS 8

void *process_client(void *p);

static volatile int serverSocket;
static volatile int endSession;

static volatile int clientsCount;
static volatile int clients[MAX_CLIENTS];

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_t tids[MAX_CLIENTS]; // Per client, a thread should be created and 'process_client' should handle that client.

/**
 * Signal handler for SIGINT.
 * Used to set flag to end server.
 */
void close_server() {
    endSession = 1;
    // fprintf(stderr, "i'm here!!!!!!!!!!!!!\n");
    // add any additional flags here you want.


    // // Gracefully exit program
    // exit(0);
}

/**
 * Cleanup function called in main after `run_server` exits.
 * Server ending clean up (such as shutting down clients) should be handled
 * here.
 */
void cleanup() {
    if (shutdown(serverSocket, SHUT_RDWR) != 0) {
        perror("shutdown():");
    }
    close(serverSocket);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != -1) {
            if (shutdown(clients[i], SHUT_RDWR) != 0) {
                perror("shutdown(): ");
            }
            close(clients[i]);
        }
    }
}

/**
 * Sets up a server connection.
 * Does not accept more than MAX_CLIENTS connections.  If more than MAX_CLIENTS
 * clients attempts to connects, simply shuts down
 * the new client and continues accepting.
 * Per client, a thread should be created and 'process_client' should handle
 * that client.
 * Makes use of 'endSession', 'clientsCount', 'client', and 'mutex'.
 *
 * port - port server will run on.
 *
 * If any networking call fails, the appropriate error is printed and the
 * function calls exit(1):
 *    - fprtinf to stderr for getaddrinfo
 *    - perror() for any other call
 */
void run_server(char *port) {
    /*QUESTION 1*/
    /*QUESTION 2*/
    /*QUESTION 3*/
    // Theoretical questions

    for (size_t i = 0; i < MAX_CLIENTS; ++i) // Initialise clients as "not existing"
        clients[i] = -1;

    // Initialise socket to run `setsockopt` on it
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    serverSocket = sock_fd;

    /*QUESTION 8*/
    int reuse_addr = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)) < 0) {
        perror(NULL);
        exit(EXIT_FAILURE);
    }
    int reuse_port = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &reuse_port, sizeof(reuse_port)) < 0) {
        perror(NULL);
        exit(EXIT_FAILURE);
    }

    /*QUESTION 4*/
    struct addrinfo hints, *res;
    memset (&hints, 0, sizeof (struct addrinfo));
    /*QUESTION 5*/
    hints.ai_family = AF_INET; // IPv4  
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE; // << We want Server socket;

    /*QUESTION 6*/
    int err = getaddrinfo(NULL, port, &hints, &res);
    if (err) {
        fprintf(stderr, "%s", gai_strerror(err));
        exit(1);
    }

    /*QUESTION 9*/
    int bind_err = bind(sock_fd, res->ai_addr, res->ai_addrlen);
    if (-1 == bind_err) {
        perror(NULL);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res); // Not needed anymore => free as early as possible

    /*QUESTION 10*/
    int listen_err = listen(sock_fd, MAX_CLIENTS); 
    if (-1 == listen_err) {
        perror(NULL);
        exit(EXIT_FAILURE);
    }

    /*QUESTION 11*/

    while (!endSession) { // while the server has not closed (endSession != 0) => giant while-loop in which we try to accept clients' connections

        int client_fd = accept(sock_fd, NULL, NULL);
        if (-1 == client_fd ) {
            perror(NULL);
            exit(EXIT_FAILURE);
        }

        pthread_mutex_lock(&mutex);

        if (clientsCount == MAX_CLIENTS) { // shut down new client, continue accepting
            shutdown(client_fd, SHUT_RDWR);
            close(client_fd);
        }
        else { // accept new client
            clientsCount ++;
            // assign id to client & 
            // a thread should be created and 'process_client' should handle that client.
            for (size_t id = 0; id < MAX_CLIENTS; ++id)
                if (clients[id] == -1) {
                    clients[id] = client_fd; 
                    pthread_create(tids + id, NULL, process_client, (void *) id);
                    break;
                }
        }

        pthread_mutex_unlock(&mutex);

    }

    for (int i = 0; i < MAX_CLIENTS; ++i) 
    if (clients[i] != -1) {
        pthread_join(tids[i], NULL);
    }
}

/**
 * Broadcasts the message to all connected clients.
 *
 * message  - the message to send to all clients.
 * size     - length in bytes of message to send.
 */
void write_to_clients(const char *message, size_t size) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != -1) {
            ssize_t retval = write_message_size(size, clients[i]);
            if (retval > 0) {
                retval = write_all_to_socket(clients[i], message, size);
            }
            if (retval == -1) {
                perror("write(): ");
            }
        }
    }
    pthread_mutex_unlock(&mutex);
}

/**
 * Handles the reading to and writing from clients.
 *
 * p  - (void*)intptr_t index where clients[(intptr_t)p] is the file descriptor
 * for this client.
 *
 * Return value not used.
 */
void *process_client(void *p) {
    pthread_detach(pthread_self());
    intptr_t clientId = (intptr_t)p;
    ssize_t retval = 1;
    char *buffer = NULL;

    while (retval > 0 && endSession == 0) {
        retval = get_message_size(clients[clientId]);
        if (retval > 0) {
            buffer = calloc(1, retval);
            retval = read_all_from_socket(clients[clientId], buffer, retval);
        }
        if (retval > 0)
            write_to_clients(buffer, retval);

        free(buffer);
        buffer = NULL;
    }

    printf("User %d left\n", (int)clientId);
    close(clients[clientId]);

    pthread_mutex_lock(&mutex);
    clients[clientId] = -1;
    clientsCount--;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "%s <port>\n", argv[0]);
        return -1;
    }

    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = close_server;
    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction");
        return 1;
    }

    run_server(argv[1]);
    cleanup();
    pthread_exit(NULL);
}
