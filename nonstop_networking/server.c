/**
 * nonstop_networking
 * CS 341 - Fall 2023
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/epoll.h>

#include "includes/dictionary.h"
#include "includes/vector.h"
#include "common.h"
#include "format.h"

dictionary *client_dict; // int -> shallow; client_fd -> client_info*
vector *file_list;       // maintain file list (server side)
dictionary *file_size;   // maintain a mapping of file (string) -> file_size (size_t = unsigned long)

struct client_info
{
    /*
    States in the DFA
    0 -> Header not parsed yet
    1 -> Header parsed
    <0 -> error codes:
        -1 = bad request
        -2 = bad file size
        -3 = no such file
    */
    int state; 
    verb req;
    char filename[MAX_FILENAME];
    char header[MAX_HEADER_LEN];
};

typedef struct client_info client_info_t;

// Functions declarations
void close_server(); // TODO
void sigint_handler(int signum);
void setup_handlers();
void setup_dir();
void setup_global_variables();
void setup_server_conn();
void setup_epoll();

void clean_client(int client_fd);
void run_client(int client_fd);
void read_header(client_info_t* c_info_ptr, int client_fd); // TODO
void exec_cmd(client_info_t* c_info_ptr, int client_fd); // TODO
void handle_errors(client_info_t* c_info_ptr, int client_fd);

// Global variables: naming convention: https://users.ece.cmu.edu/~eno/coding/CCodingStandard.html#gconstants
static char *g_temp_dir;
static char *g_port = NULL;
static int g_sock_fd; // socket exposed by the SERVER
static int g_epoll_fd;

int main(int argc, char **argv)
{
    // good luck!
    if (argc < 2)
    { // Ensure correct usage
        print_server_usage();
        exit(1);
    }

    // Ignore SIGPIPE & set up handler for SIGINT to exit the server when triggered
    setup_handlers();

    // Setup temporary directory
    setup_dir();
    // Setup global variables
    char *port = argv[1];
    setup_global_varibles(port);
    // Setup server conection
    setup_server_conn();
    // Setup `epoll`
    setup_epoll();

    // Close server
    close_server();

    return 0;
}

void sigint_handler(int signum)
{
    assert(signum == SIGINT);
    LOG("SIGINT called, closing the server");
    close_server();
}

// ------------ HELPER FUNTIONS -------------
void setup_handlers()
{

    // a.) Ignore SIGPIPE -> set handler of SIGPIPE to SIG_IGN = signal ignore; https://www.gnu.org/software/libc/manual/html_node/Basic-Signal-Handling.html
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = SA_RESTART; // restart function if interrupted by handler
    if (sigaction(SIGPIPE, &sa, NULL) < 0)
    {
        perror("sigaction on SIGPIPE failed");
        exit(1);
    }

    // b.) exit server & cleanup resources: when SIGINT is triggered
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = sigint_handler;
    if (sigaction(SIGINT, &act, NULL) < 0)
    {
        perror("sigaction on SIGINT failed");
        exit(1);
    }
}

void setup_dir()
{
    LOG("Setting up directory");
    char template[7] = "XXXXXX"; // template  must  not be a string constant, but should be delared as a character array.
    g_temp_dir = mkdtemp(template);
    print_temp_directory(g_temp_dir);
}
void setup_global_variables(char *port)
{
    LOG("Setting up global variables");
    if (!g_port)
        g_port = strdup(port);

    client_dict = int_to_shallow_dictionary_create();
    file_size = string_to_unsigned_long_dictionary_create();
    file_list = vector_create(string_copy_constructor, string_destructor, string_default_constructor);
}

void setup_server_conn()
{
    LOG("Starting setting up the server");
    g_sock_fd = socket(AF_INET, SOCK_STREAM, 0); // IPv4 TCP connection

    if (g_sock_fd == -1)
    {
        perror("Socket failed");
        exit(1);
    }

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // (!) Socket should be passive

    int gai = getaddrinfo(NULL, g_port, &hints, &res); // loopback interface addr;
    if (gai)
    {
        LOG(gai_strerror(gai));
        exit(1);
    }

    int reuse_addr = 1;
    if (setsockopt(g_sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)) < 0)
    {
        perror("SO_REUSEADDR setsockopt failed");
        exit(EXIT_FAILURE);
    }
    int reuse_port = 1;
    if (setsockopt(g_sock_fd, SOL_SOCKET, SO_REUSEPORT, &reuse_port, sizeof(reuse_port)) < 0)
    {
        perror("SO_REUSEPORT setsockopt failed");
        exit(EXIT_FAILURE);
    }

    if (bind(g_sock_fd, res->ai_addr, res->ai_addrlen) == -1)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(g_sock_fd, MAX_CLIENTS) == -1)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);
}

void setup_epoll()
{ // For reference, see demo from class: https://github.com/angrave/CS341-Lectures-FA22/blob/main/code/lec33/epoll1.c
    LOG("Starting epoll");
    g_epoll_fd = epoll_create(16);

    if (g_epoll_fd == -1)
    {
        perror("epoll_create failed");
        exit(1);
    }

    struct epoll_event ev = {.data.fd = g_sock_fd, .events = EPOLLIN}; // read

    if (epoll_ctl(g_epoll_fd, EPOLL_CTL_ADD, g_sock_fd, &ev) == -1)
    {
        perror("Tracking server event (in the interest list) with epoll failed");
        exit(1);
    }

    struct epoll_event epoll_events[MAX_EVENTS]; // Array of events that may be added to the interest list of g_epoll_fd later

    while (true)
    {
        // TODO: check timeout param
        int num_events = epoll_wait(g_epoll_fd, epoll_events, MAX_EVENTS, MAX_TIMEOUT); // How many events are currently active in the interest list  (i.e: ready for the requested I/O)

        if (num_events == 0)
            continue; // no currently active event
        if (num_events == -1)
        {
            perror("epoll_wait failed");
            exit(1);
        }

        /* Iterate over sockets only w/ active events */
        for (int i = 0; i < num_events; ++i)
        {

            int curr_event_fd = epoll_events[i].data.fd;
            if (curr_event_fd == g_sock_fd)
            { // New connection request received by the SERVER

                int client_fd = accept(g_sock_fd, NULL, NULL);
                if (client_fd == -1)
                {
                    perror("accept failed");
                    exit(1);
                }

                // Add a new event associated w/ the new connected cleintto the "interest list"
                struct epoll_event new_ev = {.data.fd = client_fd, .events = EPOLLIN};
                if (epoll_ctl(g_epoll_fd, EPOLL_CTL_ADD, client_fd, &new_ev) == -1)
                {
                    perror("Adding the new client's connection event to interest list (epoll_ctl(EPOLL_CTL_ADD)) failed");
                    exit(1);
                }

                client_info_t* c_info_ptr = calloc(1, sizeof(client_info_t));
                c_info_ptr->state = 0; // update state in DFA
                dictionary_set(client_dict, &client_fd, c_info_ptr); // map from socket handle to connection state
            }
            else 
            {
                run_client(curr_event_fd); 
            }
        }
    }
}


/* 
This method is invoked to either:
- Parse header info
- Execute command (LIST, GET, PUT or DELETE)
- Error handling on server-side: "Bad request", "Bad file size" or "No such file"
*/

void run_client(int client_fd) { 

    client_info_t* c_info_ptr = dictionary_get(client_dict, &client_dict);

    if (c_info_ptr->state == 0) { // Header not parsed completely yet
        read_header(c_info_ptr, client_fd);
    }
    else if (c_info_ptr->state == 1) {
        exec_cmd(c_info_ptr, client_fd);
    }
    else { // Error
        handle_errors(c_info_ptr, client_fd);
    }
}

void clean_client(int client_fd) {
    // Cleanup `client_fd` from `client_dict`
    client_info_t* c_info_ptr = dictionary_get(client_dict, &client_fd);
    free(c_info_ptr);
    dictionary_remove(client_dict, &client_fd);

    // Deregister client_fd from interest list
    epoll_ctl(g_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL); 
    // Shutdown & close fd
    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
}

void handle_errors(client_info_t* c_info_ptr, int client_fd) {
    // Print correct error prompt
    if (c_info_ptr ->state == -1) {
        write_to_socket(client_fd, err_bad_request, strlen(err_bad_request));
    }
    else if (c_info_ptr->state == -2) {
        write_to_socket(client_fd, err_bad_file_size, strlen(err_bad_file_size));
    }
    else {
        write_to_socket(client_fd, err_no_such_file, strlen(err_no_such_file));
    }
    //cleanup resources allocd for client
    clean_client(client_fd);
}