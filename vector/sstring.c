/**
 * vector
 * CS 341 - Fall 2023
 */
#include "sstring.h"
#include "vector.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <string.h>

struct sstring {
    // Anything you want
    char *str; 
};

sstring *cstr_to_sstring(const char *input) {
    // your code goes here
    sstring *sstr = malloc(sizeof(sstring));
    sstr->str = strdup(input);
    return sstr;
}

char *sstring_to_cstr(sstring *input) {
    // your code goes here
    assert(input);
    assert(input->str);
    char *str = strdup(input->str);
    return str;
}

int sstring_append(sstring *this, sstring *addition) {
    // your code goes here
    this->str = realloc(this->str, strlen(this->str) + 1 + strlen(addition->str) + 1);
    strcat(this->str, addition->str);
    return strlen(this->str);
}



vector *sstring_split(sstring *this, char delimiter) {
    // your code goes here
    vector* v = vector_create(string_copy_constructor, string_destructor, string_default_constructor);

    // lets use `strchr`
    char *str = this -> str;
    char *end = str + strlen(str);

    while (str < end) {
        // substring: s[str .. substrend - 1]
        char *substr_end = strchr(str, delimiter);
        if (substr_end == NULL) { // No more delimiter found => substr[str .. strlen(str) - 1]
            vector_push_back(v, str);
            str = end;
        }
        else {
            char aux = *substr_end;
            *substr_end = '\0'; // delimit substring's endpoint
            vector_push_back(v, str); 
            *substr_end = aux; // restore value
            str = substr_end + 1; // skip to advance in for-loop
        }
    }

    return v;
}

int sstring_substitute(sstring *this, size_t offset, char *target,
                       char *substitution) {
    // your code goes here

    //let's use `strstr`
    char *next = strstr(this->str + offset, target);
    if (next == NULL) return -1;
    else {
        size_t total_nbytes = strlen(this->str) + 1 - strlen(target) + strlen(substitution); 
        char *aux = malloc(total_nbytes);

        size_t first_nbytes = next - this->str;
        strncpy(aux, this->str, first_nbytes);

        size_t second_nbytes = strlen(substitution);
        strncpy(aux + first_nbytes, substitution, second_nbytes);

        size_t remaining_nbytes = total_nbytes - first_nbytes - second_nbytes;
        strncpy(aux + first_nbytes + second_nbytes, next + strlen(target), remaining_nbytes);

        aux[total_nbytes - 1] = '\0';

        free(this -> str); // Make sure you don't leak memory 
        this -> str = aux;
        return 0;
    }
}

char *sstring_slice(sstring *this, int start, int end) {
    // your code goes here

    // this->str[start .. end - 1] inclusive
    size_t nbytes_copied = end - start;
    char *str = malloc(nbytes_copied+ 1); // [end - statr] actual chars, 1 for '\0'
    strncpy(str, this->str + start, nbytes_copied);
    str[nbytes_copied] = '\0'; //zero the last byte
    return str;
}

void sstring_destroy(sstring *this) {
    // your code goes here
    assert(this); 
    free(this->str); // free all of its dynamic memory
    free(this);
}

// --------------- HELPERS --------------
vector *obsolete_sstring_append(sstring *this, char delimiter) {

    vector* v = vector_create(string_copy_constructor, string_destructor, string_default_constructor);
    int i = 0; //offset in work string
    char *walk = this->str;
    int CAPACITY = 1024;
    char *work = malloc(sizeof(char) * CAPACITY);

    while (*walk) {
        if (*walk != delimiter) {
            if (i >= CAPACITY) {
              CAPACITY *= 2;
              work = realloc(work, CAPACITY);
            }
            work[i ++] = *walk; 
        }
        else {
            if (i >= CAPACITY) {
              CAPACITY *= 2;
              work = realloc(work, CAPACITY);
            }
            work[i ++] = '\0';
            vector_push_back(v, walk);
            i = 0;
            memset(work, 0, CAPACITY);
        }

        walk ++; 

    }

    free(work);
    return v;
}
