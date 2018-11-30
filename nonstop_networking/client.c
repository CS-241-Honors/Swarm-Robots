/**
* Networking Lab
* CS 241 - Fall 2018
*/
//--------------------------------------------------------------
#include "includes/dictionary.h"
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
#include <errno.h>
//--------------------------------------------------------------
// Definitions
#define PORT1 "5000"
#define PORT2 "5001"
#define PORT3 "5002"
#define PORT4 "5003"
#define LOCAL_IP "127.0.0.1"
#define NEIGHBOR_NUM 2
#define TOTAL_BOT_NUM 4

typedef enum { 
    QUERY,
    RESPONSE,
    MOVE,
    DISCONNECT
} verb;

typedef struct message{
    int msg_id;
    verb request;
    char query_name;
    char from;
    char to;
    int direction;
    int distance;
} message; 

//--------------------------------------------------------------
// Global Variables
static char all_bot_names[TOTAL_BOT_NUM] = {'A', 'B', 'C', 'D'};
static dictionary * table;
static unsigned long msg_id;
static char * bot_name;
static char ** neighbor_names;

//--------------------------------------------------------------
// Function headers
void print_usage();
void print_invalid_msg_format();
void SIGINT_handler();
void SIGPIPE_handler();

int connect_handler_helper(char * host, char * port);
void * connect_handler(void * dummy);
void * listen_handler(void * dummy);
void do_query(char * name);

void send_name(int to_fd, char * name);
char * read_name(int from_fd);
ssize_t read_all_from_fd(int fd, char *buffer, size_t count);
ssize_t write_all_to_fd(int fd, const char *buffer, size_t count);
char * msg_before_next(char * msg);

void clear_all_table_elems();
void clear_neighbor_names(char ** _neighbor_names);
//--------------------------------------------------------------
int main(int argc, char ** argv) {
    (void) all_bot_names;
    fprintf(stderr, "%zu\n", sizeof(verb));
    if (argc != 2) {
        print_usage();    
		exit(1);
    } 
	//-----	
    // initialize global variables
	signal(SIGINT, SIGINT_handler);
	signal(SIGPIPE, SIGPIPE_handler);	
    table = dictionary_create(shallow_hash_function, shallow_compare, NULL, NULL, NULL, NULL);
    msg_id = 0;
    bot_name = argv[1];
    neighbor_names = calloc(sizeof(char *), NEIGHBOR_NUM);
	//-----	
    // Connect
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
    // fill out the routing table
    do_query(argv[1]);
	//-----	
    // clean up
    clear_all_table_elems();
    dictionary_destroy(table);
    clear_neighbor_names(neighbor_names);
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
    sleep(6);
    char * bot_num = (char *) _bot_num;
    char * port1, * port2;
    switch (bot_num[0]) {
        case 'A':
            port1 = PORT3;
            port2 = PORT2;
            break;
        case 'B':
            port1 = PORT1;
            port2 = PORT4;
            break;
        case 'C':
            port1 = PORT4;
            port2 = PORT1;
            break;
        case 'D':
            port1 = PORT2;
            port2 = PORT3;
            break;
    }
	int sock_fd1 = connect_handler_helper(LOCAL_IP, port1);
	int sock_fd2 = connect_handler_helper(LOCAL_IP, port2);
    send_name(sock_fd1,bot_num);
    send_name(sock_fd2,bot_num);
	return NULL;
}

//--------------------------------------------------------------
void * listen_handler(void * _bot_num) {
    char * bot_num = (char *) _bot_num;
    char * port;
    switch (bot_num[0]) {
        case 'A':
		    port = PORT1;	
            break;
        case 'B':
		    port = PORT2;	
            break;
        case 'C':
		    port = PORT3;	
            break;
        case 'D':
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

    if (listen(sock_fd, NEIGHBOR_NUM) != 0) {
        perror(NULL); 
        exit(1);
    }

    int processed = 0;
    while (processed < NEIGHBOR_NUM) {
        size_t client_fd = accept(sock_fd, NULL, NULL);
        char * client_name = read_name(client_fd);
        dictionary_set(table, (void *) (size_t) client_name[0], (void *) client_fd);
        neighbor_names[processed] = client_name;
        fprintf(stderr, "Bot%s successfully connects to Bot%s\n", bot_name, client_name);
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

//--------------------------------------------------------------
char * msg_before_next(char * msg) {
    if (msg == NULL) {
        return NULL;    
    }
    char * rv = calloc(1, strlen(msg) + 1);
    int itr = 0;
    while (msg[itr]) {
        if (msg[itr] == '\n') {
            return rv;    
        }
        rv[itr] = msg[itr];
        itr++;
    }
    print_invalid_msg_format();
    return NULL;
}

//--------------------------------------------------------------
void print_invalid_msg_format() {
    fprintf(stderr, "Incorrect message format.\n");    
}

//--------------------------------------------------------------
ssize_t read_all_from_fd(int fd, char *buffer, size_t count) {
    // Your Code Here
    ssize_t n = 0;
    size_t bytes_done = 0;
	while (bytes_done != count) {
        int errno_saved = errno;
        n = read(fd, buffer + bytes_done, count - bytes_done);
        if (n == 0) { //disconnected
            return 0;    
        }
        else if (n < 0) {
            if  (errno == EINTR) {
                errno = errno_saved;
                continue;    
            }
            else {
                exit(1);
            }
        }
        else { // if (n > 0) 
            bytes_done += n;
        }
	}
    return count;
}

//-------------------------------------------------------------
ssize_t write_all_to_fd(int fd, const char *buffer, size_t count) {
    // Your Code Here
    ssize_t n = 0;
    size_t bytes_done = 0;
	while (bytes_done != count) {
        int errno_saved = errno;
        n = write(fd, buffer + bytes_done, count - bytes_done);
        if (n == 0) { //disconnected
            return 0;    
        }
        else if (n < 0) {
            if  (errno == EINTR) {
                errno = errno_saved;
                continue;    
            }
            else {
                exit(1);
            }
        }
        else { // if (n > 0) 
            bytes_done += n;
        }
	}
    return count;
}

//-------------------------------------------------------------
void send_name(int to_fd, char * name) {
    write_all_to_fd(to_fd, name, 1);
}

//-------------------------------------------------------------
char * read_name(int from_fd) {
    char * name = calloc(1, 2);
    read_all_from_fd(from_fd, name, 1);    
    return name;
}

//-------------------------------------------------------------
void clear_all_table_elems() {
    for (int i=0; i < NEIGHBOR_NUM; i++) {
        if (neighbor_names[i] != NULL &&
            dictionary_contains(table, neighbor_names[i])) {
            dictionary_remove(table, neighbor_names[i]);
        }    
    }        
}

//-------------------------------------------------------------
void clear_neighbor_names(char ** _neighbor_names) {
    for (int i=0; i < NEIGHBOR_NUM; i++) {
        free(_neighbor_names[i]);    
    }    
    free(_neighbor_names);
}

//-------------------------------------------------------------
void do_query(char * curr_bot) {
    for (int i=0; i < TOTAL_BOT_NUM; i++) {
        if (all_bot_names[i] != curr_bot && 
            !dictionary_contains(table, (void *) all_bot_names[i])) {
            char * msg = calloc(sizeof(message), 1);
            msg->msg_id = 
        }
    }
}
    int msg_id;
