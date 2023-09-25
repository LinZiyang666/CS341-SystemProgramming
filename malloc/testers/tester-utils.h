/**
 * malloc
 * CS 341 - Fall 2023
 */
#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define K 1024L
#define M (1024L * 1024L)
#define G (1024 * 1024 * 1024)

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define SECONDS_PER_DAY 86400

long int rand_today();

// Check if two regions overlap
int overlap(void *r1, size_t len1, void *r2, size_t len2);

#define START_CHAR 'e'
#define END_CHAR 'l'

// Ensure that the start and end of region are writable
void verify_write(char *ptr, size_t len);

/*
 * Used with verify_write, make sure we get the same value back
 * return 1 if we get the same value back (good).
 * return 0 if we dont (bad).
 */
int verify_read(char *ptr, size_t len);

/*
 * Ensure that the whol region contain only integer c
 */
void verify(void *region, int c, size_t len);

/*
 * Ensure that the whole region contains only 0
 * Randomly choose a region
 */
void verify_clean(char *ptr, size_t len);

/*
 * Ensure that two regions [r1, r1+len) and [r2, r2+len)
 * dont overlap
 */
void verify_overlap2(void *r1, void *r2, size_t len);

/*
 * Ensure that three regions [r1, r1 + len), [r2, r2+len), and [r3, r3+len)
 * dont overlap
 */
void verify_overlap3(void *r1, void *r2, void *r3, size_t len);