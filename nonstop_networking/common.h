/**
* Networking Lab
* CS 241 - Fall 2018
*/

#pragma once
#include <stddef.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#define LOG(...)                      \
    do {                              \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n");        \
    } while (0);

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

typedef enum { GET, PUT, DELETE, LIST, V_UNKNOWN } verb;

ssize_t read_all_from_fd(int fd, char * buffer, size_t count);
ssize_t write_all_to_fd(int fd, const char * buffer, size_t count);
