/**
 * nonstop_networking
 * CS 341 - Fall 2023
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "includes/dictionary.h"
#include "includes/vector.h"
#include "common.h"
#include "format.h"


dictionary* client_dict; // int -> shallow; client_fd -> client_info*
vector* file_list; // maintain file list (server side)
dictionary* file_size; // maintain a mapping of file (string) -> file_size (size_t = unsigned long)

typedef struct client_info {
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

// Global variables: naming convention: https://users.ece.cmu.edu/~eno/coding/CCodingStandard.html#gconstants
static char* g_temp_dir;
static char* g_port = NULL;




int main(int argc, char **argv) {
    // good luck!
    if (argc < 2) {  // Ensure correct usage
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
    // Setup `epoll`
    // Close server
}


void sigint_handler(int signum) {
    assert(signum == SIGINT);
    LOG("SIGINT called, closing the server");
    close_server();
}

// ------------ HELPER FUNTIONS ------------- 
void setup_handlers() {

    // a.) Ignore SIGPIPE -> set handler of SIGPIPE to SIG_IGN = signal ignore; https://www.gnu.org/software/libc/manual/html_node/Basic-Signal-Handling.html
    struct sigaction sa; 
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = SA_RESTART; // restart function if interrupted by handler
    if (sigaction(SIGPIPE, &sa, NULL) < 0) {
        perror("sigaction on SIGPIPE failed");
        exit(1);
    }


    // b.) exit server & cleanup resources: when SIGINT is triggered
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = sigint_handler;
    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction on SIGINT failed");
        exit(1);
    }
}

void setup_dir() {
    char template[7] = "XXXXXX"; // template  must  not be a string constant, but should be delared as a character array.
    g_temp_dir = mkdtemp(template);
    print_temp_directory(g_temp_dir);
}
void setup_global_variables(char *port) {
    if (!g_port)
        g_port = strdup(port);
    
    client_dict = int_to_shallow_dictionary_create();
    file_size = string_to_unsigned_long_dictionary_create();
    file_list = vector_create(string_copy_constructor, string_destructor, string_default_constructor);
    
    


}