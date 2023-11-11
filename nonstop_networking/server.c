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

#include "includes/dictionary.h"
#include "includes/vector.h"
#include "common.h"
#include "format.h"

dictionary *client_dict; // int -> shallow; client_fd -> client_info*
vector *file_list;       // maintain file list (server side)
dictionary *file_size;   // maintain a mapping of file (string) -> file_size (size_t = unsigned long)

typedef struct client_info
{
    int state;
    verb req;
    char filename[MAX_FILENAME];
    char header[MAX_HEADER_LEN];
};

// Functions declarations
void close_server(); // TODO
void sigint_handler(int signum);
void setup_handlers();
void setup_dir();
void setup_global_variables();
void setup_server_conn();

void clean_client(int client_fd); // TODO: after executing a command

// Global variables: naming convention: https://users.ece.cmu.edu/~eno/coding/CCodingStandard.html#gconstants
static char *g_temp_dir;
static char *g_port = NULL;
static int g_sock_fd; // socket exposed by the SERVER

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
    // Close server
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

    if (bind(g_sock_fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(g_sock_fd, MAX_CLIENTS) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    
    freeaddrinfo(res);
}