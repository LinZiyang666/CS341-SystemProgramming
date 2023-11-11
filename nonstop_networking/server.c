/**
 * nonstop_networking
 * CS 341 - Fall 2023
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <dirent.h>
#include <assert.h>

#include "includes/dictionary.h"
#include "includes/vector.h"
#include "common.h"
#include "format.h"

static dictionary *client_dict; // int -> shallow; client_fd -> client_info*
static vector *file_list;       // maintain file list (server side)
static dictionary *file_size;   // maintain a mapping of file (string) -> file_size (size_t = unsigned long)

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
    verb cmd; // GET, LIST, PUT or GET
    char filename[MAX_FILENAME];
    char header[MAX_HEADER_LEN];
};

typedef struct client_info client_info_t;

// Functions declarations
int rm_unempty_dir(char* path);
int is_directory(char* path);

void close_server();
void sigint_handler(int signum);
void setup_handlers();
void setup_dir();
void setup_global_variables();
void setup_server_conn();
void setup_epoll();

void clean_client(int client_fd);
void run_client(int client_fd);
void read_header(client_info_t *c_info_ptr, int client_fd);

// RET: 0 if successful, 1 if failed
int exec_get(client_info_t *c_info_ptr, int client_fd);    
int exec_put(client_info_t *c_info_ptr, int client_fd);    
int exec_list(int client_fd);   
int exec_delete(client_info_t *c_info_ptr, int client_fd); 
void exec_cmd(client_info_t *c_info_ptr, int client_fd);

void handle_errors(client_info_t *c_info_ptr, int client_fd);
void epoll_set_client_WR(int client_fd);

void write_get_to_client(int client_fd, size_t size, FILE* read_file);

int read_put_from_client(client_info_t *c_info_ptr, int client_fd);


// Global variables: naming convention: https://users.ece.cmu.edu/~eno/coding/CCodingStandard.html#gconstants
static char *g_temp_dir;
static char *g_port = NULL;
static int g_sock_fd; // socket exposed by the SERVER
static int g_epoll_fd = -1;

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
    setup_global_variables(port);
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
        close_server();
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
        fprintf(stderr, "%s\n", gai_strerror(gai));
        close_server();
        exit(1);
    }

    int reuse_addr = 1;
    if (setsockopt(g_sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)) < 0)
    {
        perror("SO_REUSEADDR setsockopt failed");
        close_server();
        exit(EXIT_FAILURE);
    }
    int reuse_port = 1;
    if (setsockopt(g_sock_fd, SOL_SOCKET, SO_REUSEPORT, &reuse_port, sizeof(reuse_port)) < 0)
    {
        perror("SO_REUSEPORT setsockopt failed");
        close_server();
        exit(EXIT_FAILURE);
    }

    if (bind(g_sock_fd, res->ai_addr, res->ai_addrlen) == -1)
    {
        perror("bind failed");
        close_server();
        exit(EXIT_FAILURE);
    }

    if (listen(g_sock_fd, MAX_CLIENTS) == -1)
    {
        perror("listen failed");
        close_server();
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

                client_info_t *c_info_ptr = calloc(1, sizeof(client_info_t));
                c_info_ptr->state = 0;                               // update state in DFA
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

void run_client(int client_fd)
{
    LOG("Running client");
    client_info_t *c_info_ptr = dictionary_get(client_dict, &client_fd);

    if (c_info_ptr->state == 0)
    { // Header not parsed completely yet
        read_header(c_info_ptr, client_fd);
    }
    else if (c_info_ptr->state == 1)
    {
        exec_cmd(c_info_ptr, client_fd);
    }
    else
    { // Error
        handle_errors(c_info_ptr, client_fd);
    }
}

void clean_client(int client_fd)
{
    // Cleanup `client_fd` from `client_dict`
    client_info_t *c_info_ptr = dictionary_get(client_dict, &client_fd);
    free(c_info_ptr);
    dictionary_remove(client_dict, &client_fd);

    // Deregister client_fd from interest list
    epoll_ctl(g_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
    // Shutdown & close fd
    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
}

void handle_errors(client_info_t *c_info_ptr, int client_fd)
{
    // Print correct error prompt
    if (c_info_ptr->state == -1)
    {
        write_to_socket(client_fd, err_bad_request, strlen(err_bad_request));
    }
    else if (c_info_ptr->state == -2)
    {
        write_to_socket(client_fd, err_bad_file_size, strlen(err_bad_file_size));
    }
    else
    {
        write_to_socket(client_fd, err_no_such_file, strlen(err_no_such_file));
    }
    // cleanup resources allocd for client
    clean_client(client_fd);
}

void read_header(client_info_t *c_info_ptr, int client_fd)
{
    size_t bytes_read = read_header_from_socket(client_fd, c_info_ptr->header, MAX_HEADER_LEN);
    if (bytes_read == 1024) // The maximum header length (header is part before data) for both the request and response is 1024 bytes => Bad request (malformed)
    {
        c_info_ptr->state = -1;         // mark the error
        epoll_set_client_WR(client_fd); // to write the error to the client
        return;
    }

    if (!strncmp(c_info_ptr->header, "GET", 3))
    {
        c_info_ptr->cmd = GET;
        strcpy(c_info_ptr->filename, c_info_ptr->header + strlen("GET\n"));
        c_info_ptr->header[strlen(c_info_ptr->header) - 1] = '\0';
    }
    else if (!strncmp(c_info_ptr->header, "PUT", 3))
    {
        c_info_ptr->cmd = PUT;
        strcpy(c_info_ptr->filename, c_info_ptr->header + strlen("PUT\n"));
        c_info_ptr->header[strlen(c_info_ptr->header) - 1] = '\0';

        int err_put = read_put_from_client(c_info_ptr, client_fd);
        if (err_put)
        {                                   // Bad file size
            c_info_ptr->state = -2;         // mark state
            epoll_set_client_WR(client_fd); // to write the error to the client
            return;
        }
    }
    else if (!strncmp(c_info_ptr->header, "DELETE", 6))
    {
        c_info_ptr->cmd = DELETE;
        strcpy(c_info_ptr->filename, c_info_ptr->header + strlen("DELETE\n"));
        c_info_ptr->header[strlen(c_info_ptr->header) - 1] = '\0';
    }
    else if (!strncmp(c_info_ptr->header, "LIST", 4))
    {
        // Notice there is no new line at the end of the list.
        c_info_ptr->cmd = LIST;
    }
    else
    { // Nonexistent verb => Bad request
        print_invalid_response();
        c_info_ptr->state = -1;         // mark the error
        epoll_set_client_WR(client_fd); // to write the error to the client
        return;
    }

    // Now we have succesfully parsed the header => set state = 1
    c_info_ptr->state = 1;
    epoll_set_client_WR(client_fd); // for later to write server's response to the client
}

void epoll_set_client_WR(int client_fd)
{ // Changed client's event from READ to WRITE => from EPOLLIN to EPOLLOUT
    struct epoll_event changed_ev = {.data.fd = client_fd, .events = EPOLLOUT};
    epoll_ctl(g_epoll_fd, EPOLL_CTL_MOD, client_fd, &changed_ev);
}

void exec_cmd(client_info_t *c_info_ptr, int client_fd)
{
    LOG("exec cmd");
    // NOTE: on errors, early return and we will `clean_client` when handling errors (`handle_errors`)
    if (c_info_ptr->cmd == GET)
    {
        int err_get = exec_get(c_info_ptr, client_fd);
        if (err_get)
            return;
    }
    else if (c_info_ptr->cmd == PUT)
    {
        LOG("exec PUT");
        write_to_socket(client_fd, OK, strlen(OK)); // have already parsed PUT answer from client, as we needed to check if "Bad file size" in `read_header`
    }
    if (c_info_ptr->cmd == LIST)
    {
        int err_list = exec_list(client_fd);
        if (err_list)
            return;
    }
    if (c_info_ptr->cmd == DELETE)
    {
        int err_delete = exec_delete(c_info_ptr, client_fd);
        if (err_delete)
            return;
    }

    clean_client(client_fd);
}

int exec_get(client_info_t *c_info_ptr, int client_fd) { // read from `dir/filename` and write OK\n[size][file_content] to client socket
    LOG("exec GET");
    int filename_len = strlen(g_temp_dir) + 1 + strlen(c_info_ptr->filename) + 1; // `%s/%s`
    char filepath[filename_len];
    memset(filepath, 0, filename_len);
    sprintf(filepath, "%s/%s", g_temp_dir, c_info_ptr->filename);

    FILE* read_file = fopen(filepath, "r");
    if (read_file == NULL) // nonexistent file or could not open
     {
        c_info_ptr->state = -3; // No such file error
        return 1;
     }

     write_to_socket(client_fd, OK, strlen(OK)); // OK\n
     size_t size = *(size_t*)dictionary_get(file_size, c_info_ptr->filename);
     write_to_socket(client_fd, (char*) &size, sizeof(size_t)); //[size]

     write_get_to_client(client_fd, size, read_file); 

     fclose(read_file);

    return 0;
}


void write_get_to_client(int client_fd, size_t size, FILE* read_file) {
    size_t bytes_wrote = 0;
    while (bytes_wrote < size) {
        size_t new_bytes_wrote = get_min(size - bytes_wrote, MAX_HEADER_LEN);
        char buffer[MAX_HEADER_LEN + 1] = {0};
        fread(buffer,  1, new_bytes_wrote, read_file); // read from `read_file` to buffer
        write_to_socket(client_fd, buffer, new_bytes_wrote); // write from buffer to client_fd
        bytes_wrote += new_bytes_wrote;
    }
}

// NOTE: LIST cannot fail
int exec_list(int client_fd) { // read from `dir/filename` and write OK\n[size][files_from_dir] to client socket
    LOG("exec LIST");
    size_t size = 0;

     write_to_socket(client_fd, OK, strlen(OK)); // OK\n
     VECTOR_FOR_EACH(file_list, filename, {
        size += strlen(filename) + 1;
     });

     if (size) size --; // Each filename has \n appended to it, if it is not the last file

     write_to_socket(client_fd, (char*) &size, sizeof(size_t)); //[size]

     VECTOR_FOR_EACH(file_list, filename, {
        write_to_socket(client_fd, filename, strlen(filename));
        if (_it != _iend - 1) // not the last file in the `file_list` vector =>  Notice there is no new line at the end of the list.
            write_to_socket(client_fd, "\n", 1);
     });

    return 0;
}

int exec_delete(client_info_t *c_info_ptr, int client_fd) { // read `dir/filename`, print OK\n if exists or mark status = 3 to client_fd (No such file in c_info_ptr) , and remove the file;
// Check if `dir/filename` exists by iterating through `file_list`; if found (i < vector_size(file_list)): erase from vector & remove from file_size dict
    LOG("exec DELETE");
    int filename_len = strlen(g_temp_dir) + 1 + strlen(c_info_ptr->filename) + 1; // `%s/%s`
    char filepath[filename_len];
    memset(filepath, 0, filename_len);
    sprintf(filepath, "%s/%s", g_temp_dir, c_info_ptr->filename);

    if (remove(filepath) == -1) {
        c_info_ptr->state = -3; // No such file
        return 1; // Return error
    }

    size_t id = 0;
    VECTOR_FOR_EACH(file_list, filename, { // Search for filename == c_info_ptr->filename
        if (strcmp(filename, c_info_ptr->filename) != 0)
            id ++;
        else break;
    });

    if (id == vector_size(file_list)) // File was not found
    {
        c_info_ptr->state = -3; // No such file
        return 1; // Return error
    }

    write_to_socket(client_fd, OK, strlen(OK)); // OK\n -> if file was found

    // Remove the file from our Data Structures
    vector_erase(file_list, id);
    dictionary_remove(file_size, c_info_ptr->filename);

    return 0;
}


// Invoked:
// a.) When server is being closed
// b.) When process receives SIGINT
void close_server() {  

    if (g_epoll_fd != -1)
        close(g_epoll_fd);

    vector_destroy(file_list);

    vector* c_infos = dictionary_values(client_dict);

    VECTOR_FOR_EACH(c_infos, c_info, {
        free(c_info);
    });

    vector_destroy(c_infos);
    dictionary_destroy(client_dict);

    if (rmdir(g_temp_dir) == -1) { // dir may not be empty
        rm_unempty_dir(g_temp_dir); // clean up any files stored in this directory, and then delete the directory itself.
    }

    dictionary_destroy(file_size);
    exit(1);
}

// Simplified version of: https://stackoverflow.com/questions/2256945/removing-a-non-empty-directory-programmatically-in-c-or-c
int rm_unempty_dir(char *dir_path) {

    //TODO: allocate `buff` on the HEAP, as the recursion may be deep and the stack may not be enough
    //TODO: error checking for `unlink`

    DIR* dp = opendir(dir_path);
    struct dirent* de = NULL;
    char buff[2 * MAX_FILENAME + 1] = {0}; //see `dirent` man pages => strlen(de->dname) <= MAX_FILENMAE, `dir_path` <= MAX_FILENAME

    while ((de = readdir(dp))) {
        sprintf(buff, "%s/%s", dir_path, de->d_name);
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) /* Skip the names "." and ".." as we don't want to recurse on them. */
            continue;
        
        if (is_directory(buff)) { // recurse
            rm_unempty_dir(buff);
        }
        else  { // is file => unlink
            unlink(buff);
        }
    }

    closedir(dp);
    rmdir(dir_path);

    return 0;
}

// RET: 0 if successful, else error_code 1 if failed
// Called from `read_header`; TODO: think how to call it from `exec_cmd`
// Reads the data after the PUT header that the client sent
int read_put_from_client(client_info_t *c_info_ptr, int client_fd)
{

    /*
    Format of client request to server (EXAMPLE):
        PUT prison_break_s05_e01.mp4\n
        [size]some call it prison break others call it privilege escalation ...
    */

    int filename_len = (strlen(g_temp_dir) + 1 + strlen(c_info_ptr->filename)) + 1; // `%s/%s` w/ '\0' at the end
    char filepath[filename_len];
    memset(filepath, 0, filename_len);

    sprintf(filepath, "%s/%s", g_temp_dir, c_info_ptr->filename); // write the filepath in the correct format

    // we will check if read_file == NULL, and if so: file could not be opened => it did not exist before, but was created by `write_file` => add it to the `files` list
    FILE *read_file = fopen(filepath, "r");
    FILE *write_file = fopen(filepath, "w"); // If a PUT request is called with an existing file. overwrite the file

    if (write_file == NULL)
    {
        perror("fopen() with w mode failed");
        return 1;
    }

    size_t size = 0;
    read_from_socket(client_fd, (char *)&size, sizeof(size_t));
    size_t bytes_read = 0;

    while (bytes_read < size + 5)
    {

        size_t bytes_to_read = get_min(size + 5 - bytes_read, MAX_HEADER_LEN);
        char buffer[MAX_HEADER_LEN + 1] = {0};

        ssize_t bytes_read_from_socket = read_from_socket(client_fd, buffer, bytes_to_read);
        if (bytes_read_from_socket == -1)
            continue;
        if (bytes_read_from_socket == 0)
            break;

        // Write the bytes read from the socket to the local file (on the server)
        fwrite(buffer, 1, bytes_read_from_socket, write_file); // TODO: check order of `size` and `nmemb` params
        bytes_read += bytes_read_from_socket;
    }

    // If a request fails
    if (is_error(bytes_read, size))
    {
        // delete the file
        remove(filepath);
        return 1;
    }

    fclose(write_file);
    if (read_file == NULL)
    { // file could not be opened => it did not exist before, but was created by `write_file` => add it to the `files` list
        vector_push_back(file_list, c_info_ptr->filename);
    }
    else
        fclose(read_file);

    // Modify the file size of the file that we've just written

    dictionary_set(file_size, c_info_ptr->filename, &size); //NOTE: store address of size_t => (size_t*), make sure to deref. it correctly
    return 0;
}

int is_directory(char* path) { // Inspired from: https://stackoverflow.com/questions/3828192/checking-if-a-directory-exists-in-unix-system-call
    struct stat st; 
    if (stat(path, &st) == -1) {
        perror("is_directory stat failed");
        exit(1);
    }
    return S_ISDIR(st.st_mode);
}