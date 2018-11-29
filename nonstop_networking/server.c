/**
* Networking Lab
* CS 241 - Fall 2018
*/
//--------------------------------------------------------------
#include "common.h"
#include "format.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
//--------------------------------------------------------------
// Global Variables & macros
#define MAX_CLIENTS 65535
//--------------------------------------------------------------
int make_server_socket(char * port);
//--------------------------------------------------------------
int main(int argc, char **argv) {
    if (argc != 2) {
        printf("./server <port>\n");    
        exit(1);
    }
    int server_fd = make_server_socket(argv[1]);
    make_repository();
    (void) server_fd;
    // real stuff here
    return 0;
}
//--------------------------------------------------------------
int make_server_socket(char * port) {
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int s = getaddrinfo(NULL, port, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo:%s\n", gai_strerror(s));
        exit(1);
    }

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    int optval = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    if (bind(sock_fd, result->ai_addr, result->ai_addrlen) != 0) {
        exit(1);
    }
    if (listen(sock_fd, MAX_CLIENTS) != 0) {
        exit(1);    
    }

    return sock_fd;
}

//--------------------------------------------------------------

//--------------------------------------------------------------

//--------------------------------------------------------------

//--------------------------------------------------------------

//--------------------------------------------------------------

//--------------------------------------------------------------

//--------------------------------------------------------------

//--------------------------------------------------------------

