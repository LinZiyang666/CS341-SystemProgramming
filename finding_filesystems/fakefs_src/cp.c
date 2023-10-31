/**
 * finding_filesystems
 * CS 341 - Fall 2023
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE
#endif

#include "../minixfs.h"
#include "../minixfs_utils.h"
#include "dictionary.h"
#include "vector.h"

#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <regex.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "fakefs.h"
#include "fakefs_utils.h"

#define BUFSIZE 256

int main(int argc, char **argv) {
    if (argc != 3) {
        // Our copy is basic, prolly doesn't need to be much more
        return 1;
    }
    int src = open(argv[1], O_RDONLY);
    int dst = creat(argv[2], O_WRONLY);
    char buf[BUFSIZE];
    ssize_t ret;
    if (src >= 0 && dst >= 0) {
        while ((ret = read(src, buf, BUFSIZE)) > 0) {
            if ((ret = write(dst, buf, ret)) < 0) {
                break;
            }
        }
    } else {
        ret = 1;
    }
    close(src);
    close(dst);
    // Update error codes
    return ret ? 1 : 0;
}