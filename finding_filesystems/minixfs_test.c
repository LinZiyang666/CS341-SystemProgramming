/**
 * finding_filesystems
 * CS 341 - Fall 2023
 */
#include "minixfs.h"
#include "minixfs_utils.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define NDEBUG

// To meet PRECOND, do `rm test.fs && make testfs` to restart the state of the filesystem

void test_write_hello_world() {
    file_system *fs = open_fs("test.fs");
    off_t off = 0;
    char *test = "Hello World";
    size_t bytes_wrote = minixfs_write(fs, "/goodies/hello.txt", test, strlen(test), &off);
    printf("bytes_wrote :%ld\n", bytes_wrote);
}

void test_write_long_string() { // PRECOND: assume "cat /goodies/hello.txt == "Hello World""
    file_system *fs = open_fs("test.fs");
    off_t off = 0;
    char *test = "This is a string that's too long, should make the file longer";
    size_t bytes_wrote = minixfs_write(fs, "/goodies/hello.txt", test, strlen(test), &off);
    printf("bytes_wrote :%ld\n", bytes_wrote);
}

void test_write_override_part_of_string() { // PRECOND: assume "cat /goodies/hello.txt == "Hello World""
    file_system *fs = open_fs("test.fs");
    off_t off = 0;
    char *test = "Byebi";
    size_t bytes_wrote = minixfs_write(fs, "/goodies/hello.txt", test, strlen(test), &off);
    printf("bytes_wrote :%ld\n", bytes_wrote);

}


// -----------
void read_nonzero_offset() { 
// Should be called from `test_write_nonzero_offset_file_size_does_not_change`
     
    file_system *fs = open_fs("test.fs");
    off_t off = 0; // read from start;
    char *buf = calloc(1, 100);
    size_t bytes_read = minixfs_read(fs, "/goodies/hello.txt", buf, strlen("Hello Rares!"), &off);
    char *obtained = buf;
    char *expected = "Hello Rares!";

    assert( 0 == strcmp(obtained, expected) );
    printf("bytes_read :%ld\n", bytes_read);
    printf("off: %ld; expected: 12 (after !)\n", off); // Has the offset move enough
    assert(off == 12);
    assert(bytes_read == 12);
    close_fs(&fs);
}
 // Tests both read & write
void test_write_nonzero_offset_file_size_does_not_change() { // PRECOND: assume "cat /goodies/hello.txt == "Hello World""
    file_system *fs = open_fs("test.fs");
    off_t off = 6;
    char *test = "Rares";
    size_t bytes_wrote = minixfs_write(fs, "/goodies/hello.txt", test, strlen(test), &off);
    printf("bytes_wrote: %ld\n", bytes_wrote);
    assert(bytes_wrote == 5); 
    assert(off == 11); // Has the offset move enough
    
    // REVIEW: After writing successfully, *off should be incremented by the number of bytes written.

    close_fs(&fs);
    read_nonzero_offset();
}

// -----------
void read_nonzero_offset_file_size_changes() {
// Should be called from `test_write_nonzero_offset_file_size_changes`
     
    size_t expected_read_size = strlen("Hello Rares are multe mere!");

    file_system *fs = open_fs("test.fs");
    off_t off = 0; // read from start;
    char *buf = calloc(1, 100);
    size_t bytes_read = minixfs_read(fs, "/goodies/hello.txt", buf, expected_read_size, &off);
    char *obtained = buf;
    char *expected = "Hello Rares are multe mere!";

    assert( 0 == strcmp(obtained, expected) );
    printf("bytes_read :%ld\n", bytes_read);
    assert((size_t) off == expected_read_size);
    assert(bytes_read == expected_read_size);
    close_fs(&fs);

}

// -----------

 // Tests both read & write
void test_write_nonzero_offset_file_size_changes() { // PRECOND: assume "cat /goodies/hello.txt == "Hello World""
    file_system *fs = open_fs("test.fs");
    off_t off = 6;
    char *test = "Rares are multe mere!";
    size_t bytes_wrote = minixfs_write(fs, "/goodies/hello.txt", test, strlen(test), &off);
    printf("bytes_wrote: %ld\n", bytes_wrote);
    assert(bytes_wrote == strlen(test)); 
    assert(off == strlen("Hello Rares are multe mere!")); // Has the offset move enough
    
    // REVIEW: After writing successfully, *off should be incremented by the number of bytes written.
    close_fs(&fs);
    read_nonzero_offset_file_size_changes();
}

// -----------

void test_read_in_the_middle() {
    // PRECOND: assume "cat /goodies/hello.txt == "Hello world""
    // Try to read `Wor`

    const char* expected_read = "wor";


    file_system *fs = open_fs("test.fs");
    off_t off = strlen("Hello "); // read from "W" (after ' ');
    char *buf = calloc(1, 100);
    size_t bytes_read = minixfs_read(fs, "/goodies/hello.txt", buf, 3, &off);
    char *obtained = buf;

    assert( 0 == strcmp(obtained, expected_read) );
    printf("bytes_read: %ld\n", bytes_read);
    assert((size_t) off == strlen("Hello wor"));
    assert(bytes_read == 3);
    close_fs(&fs);

}

// -----------
void continue_writing_over_test() { // Should be called from `example_write_test`
// This also enlarges the file! Careful for that edge case

    file_system *fs = open_fs("test.fs");
    off_t off = strlen("Hello");
    char *test = " Mara!";
    size_t bytes_wrote = minixfs_write(fs, "/goodies/newfile.txt", test, strlen(test), &off);
    printf("bytes_wrote: %ld\n", bytes_wrote);
    assert(bytes_wrote == strlen(test)); 
    assert(off == strlen("Hello Mara!")); // Has the offset move enough
    
    // REVIEW: After writing successfully, *off should be incremented by the number of bytes written.

    close_fs(&fs);

    // EXTRA: run `./fakefs test.fs cat test.fs/goodies/newfile.txt` and you should obtain "Hello Mara!"
}

void example_write_test() {
    // Writes to a new test file /goodies/newfile.txt -> "Hello"
    // Afterwards, calls `continue_writing_over_test` to say hello to Mara -> "Hello Mara!"

    file_system *fs = open_fs("test.fs");
    off_t off = 0;
    char *test = "Hello";
    size_t bytes_wrote = minixfs_write(fs, "/goodies/newfile.txt", test, strlen(test), &off);
    printf("bytes_wrote: %ld\n", bytes_wrote);
    assert(bytes_wrote == strlen("Hello")); 
    assert(off == strlen("Hello")); // Has the offset move enough
    
    // REVIEW: After writing successfully, *off should be incremented by the number of bytes written.

    close_fs(&fs);
    continue_writing_over_test();
}

// -----------

int main(int argc, char *argv[]) {
    // test_write_hello_world();
    // test_write_long_string(); 
    // test_write_override_part_of_string(); 
    // test_write_nonzero_offset_file_size_does_not_change();
    // test_write_nonzero_offset_file_size_changes();
    // test_read_in_the_middle();
    example_write_test();  // https://edstem.org/us/courses/41378/discussion/3772511?comment=8696614
}