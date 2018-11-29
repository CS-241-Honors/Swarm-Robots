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
#include <pthread.h>
//--------------------------------------------------------------
#define PORT1 "5000"
#define PORT2 "5001"
#define PORT3 "5002"
#define PORT4 "5003"
#define LOCAL_IP "127.0.0.1"
#define MAX_NEIGHBOR 3
#define NEIGHBOR_NUM 2
//--------------------------------------------------------------
void print_usage();
void SIGINT_handler();
void SIGPIPE_handler();

int connect_handler_helper(char * host, char * port);
void * connect_handler(void * dummy);
void * listen_handler(void * dummy);
//--------------------------------------------------------------
int main(int argc, char ** argv) {
    if (argc != 2) {
        print_usage();    
		exit(1);
    } 
	signal(SIGINT, SIGINT_handler);
	signal(SIGPIPE, SIGPIPE_handler);	
	//-----	
    pthread_t connect_thread;
    pthread_t listen_thread;

	if (pthread_create(&connect_thread, NULL, connect_handler, (void*) argv[1]) ||
        pthread_create(&listen_thread, NULL, listen_handler, (void *) argv[1])) {
        fprintf(stderr, "Bot%s fails to join the network.\n", argv[1]);
		exit(1);
    }
    void * dummy = NULL;
    pthread_join(connect_thread, &dummy);
    pthread_join(listen_thread, &dummy);
	fprintf(stderr, "Bot%s successfully joins the network.\n", argv[1]);
	//-----	
    return 0;    
}

//--------------------------------------------------------------
void print_usage() {
    fprintf(stderr, "./client bot_number\n");    
}

//--------------------------------------------------------------
int connect_handler_helper(char * host, char * port) {
	struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int s = getaddrinfo(host, port, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(1);
    }

    int sock_fd = socket(hints.ai_family, hints.ai_socktype, 0);
    if (sock_fd == -1) {
        fprintf(stderr, "client: fail to create a socket.\n");
        exit(1);
    }

    int ok = connect(sock_fd, result->ai_addr, result->ai_addrlen);
    if (ok == -1) {
        fprintf(stderr, "client: fail to connect.\n");
        exit(1);
    }
    freeaddrinfo(result);
    return sock_fd;
}

//--------------------------------------------------------------
void * connect_handler(void * _bot_num) {
    sleep(10);
    char * bot_num = (char *) _bot_num;
    char * port1, * port2;
    switch (bot_num[0]) {
        case '1':
            port1 = PORT3;
            port2 = PORT2;
            break;
        case '2':
            port1 = PORT1;
            port2 = PORT4;
            break;
        case '3':
            port1 = PORT4;
            port2 = PORT1;
            break;
        case '4':
            port1 = PORT2;
            port2 = PORT3;
            break;
    }
	int sock_fd1 = connect_handler_helper(LOCAL_IP, port1);
	int sock_fd2 = connect_handler_helper(LOCAL_IP, port2);
	(void) sock_fd1;
	(void) sock_fd2;
	return NULL;
}

//--------------------------------------------------------------
void * listen_handler(void * _bot_num) {
    char * bot_num = (char *) _bot_num;
    char * port;
    switch (bot_num[0]) {
        case '1':
		    port = PORT1;	
            break;
        case '2':
		    port = PORT2;	
            break;
        case '3':
		    port = PORT3;	
            break;
        case '4':
		    port = PORT4;	
            break;
    }
	//-----
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    int s = getaddrinfo(NULL, port, &hints, &result); 
    if (s != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s)); 
            exit(1);
    }
    
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    int optval = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    if (bind(sock_fd, result->ai_addr, result->ai_addrlen) != 0) {
        perror(NULL); 
        exit(1);
    }

    if (listen(sock_fd, MAX_NEIGHBOR) != 0) {
        perror(NULL); 
        exit(1);
    }

    int processed = 0;
    while (processed < NEIGHBOR_NUM) {
        int client_fd = accept(sock_fd, NULL, NULL);
        (void) client_fd;
        processed++;
    }
    freeaddrinfo(result);
    return NULL;    
}

//--------------------------------------------------------------
void SIGINT_handler() {
    
}

//--------------------------------------------------------------
void SIGPIPE_handler() {
    
}

