#include "common.h"
#include "dictionary.h"
#include "format.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "pthread.h"
#include "set.h"


//----------------------------------------------------------------------------------
// Definitions
#define LOCAL_IP "127.0.0.1"
#define BACKLOG 16
#define APORT "9000"
#define BPORT "9001"
#define CPORT "9002"
#define DPORT "9003"

//----------------------------------------------------------------------------------
// Global variables
static char my_name;
static char * my_port;
static dictionary * table;
static vector * seek_ports;

//----------------------------------------------------------------------------------
// Structures
/**
 * te_metadata = Table Entry Metadata
 * <key, value> == <char, te_metadata>
 */
typedef struct value{
    int direct;
    int fd;
} value;

typedef struct message{
    int id;
    char to;
    char from;
    int type;
    int subtype;
} message; 


//----------------------------------------------------------------------------------
/**
 * populates seek_ports vector with the neighbors to connect to
 * 
 */
void populate_seek_ports(){
    switch(my_name){
        case 'A':
            vector_push_back(seek_ports, BPORT);
            vector_push_back(seek_ports, CPORT);
            break;
        case 'B':
            vector_push_back(seek_ports, APORT);
            vector_push_back(seek_ports, DPORT);
            break;
        case 'C':
            vector_push_back(seek_ports, APORT);
            vector_push_back(seek_ports, DPORT);
            break;
        case 'D':
            vector_push_back(seek_ports, BPORT);
            vector_push_back(seek_ports, CPORT);
            break;
        default:
            printf("Did not find appropriate port\n");
            exit(1);
    }

}

//----------------------------------------------------------------------------------
/**
 * get port
 * 
 */
char * get_port(){
    switch(my_name){
        case 'A':
            my_port = "9000";
            break;
        case 'B':
            my_port = "9001";
            break;
        case 'C':
            my_port = "9002";
            break;
        case 'D':
            my_port = "9003";
            break;
        default:
            printf("Did not find appropriate port\n");
            exit(1);
    }

    return my_port;
}

//----------------------------------------------------------------------------------
/**
 * start listening for incoming connections in a loop
 * store any open connections into a table
 */

void * open_ears(void * ignore){
    (void)ignore;

    struct addrinfo hints, * res;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int s = getaddrinfo(NULL, my_port, &hints, &res); 
    if (s != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s)); 
            exit(1);
    }
    
    int ear_fd = socket(AF_INET, SOCK_STREAM, 0);
    int optval = 1;
    setsockopt(ear_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    if (bind(ear_fd, res->ai_addr, res->ai_addrlen) != 0) {
        perror(NULL); 
        exit(1);
    }

    if (listen(ear_fd, BACKLOG) != 0) {
        perror(NULL); 
        exit(1);
    }

    while(1){
        // accept neighbor
        printf("Now accepting...\n");
        int new_neighbor_fd = accept(ear_fd, NULL, NULL);
        // read the init message
        char init_buffer[sizeof(message)];
        printf("Now reading from new neighbor...\n");
        read(new_neighbor_fd, init_buffer, sizeof(message));
        message * message_data = (message *)init_buffer;
        char new_neighbor_name = message_data->from;
        // fill out value
        value * new_value = (value *) malloc(sizeof(value));
        new_value->fd = new_neighbor_fd;
        new_value->direct = 1;
        // throw it in table
        dictionary_set(table, (void *)(size_t)new_neighbor_name, (void *)new_value);

        printf("Robot %c is now added to my dictionary!\n", new_neighbor_name);
    }

    return NULL;
}

//----------------------------------------------------------------------------------
/**
 * connect to this port and ip
 * 
 */
 void connect_to(char * host, char * port){
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


    char init_buffer[sizeof(message)];
    printf("Now reading from new neighbor...\n");
    read(sock_fd, init_buffer, sizeof(message));
    message * message_data = (message *)init_buffer;
    char new_neighbor_name = message_data->from;
    // fill out value
    value * new_value = (value *) malloc(sizeof(value));
    new_value->fd = sock_fd;
    new_value->direct = 1;
    // throw it in table
    if (dictionary_contains(table, (void *)(size_t)new_neighbor_name)){
        return 'Z';
    }
    dictionary_set(table, (void *)(size_t)new_neighbor_name, (void *)new_value);
    printf("Robot %c is now added to my dictionary!\n", new_neighbor_name);


    return 
 }

//----------------------------------------------------------------------------------
/**
 * seek out pre-defined neighbors
 * 
 */

void * seek_out(void * ignore){
    (void)ignore;


    for(size_t i = 0; i < vector_size(seek_ports); i++){
        char possibly_new_neighbor = connect_to(LOCAL_IP, vector_get(seek_ports, i));
        if (dictionary(s))
    }

    return NULL;
}


//----------------------------------------------------------------------------------
/**
 * main function
 * 
 */
int main(int argc, char * argv[]){
    my_name =  argv[1][0];
    printf("My name is %c\n", my_name);
    my_port = get_port();
    printf("My port is %s\n", my_port);

    seek_ports = vector_create(string_copy_constructor,
                      string_destructor,
                      string_default_constructor);

    populate_seek_ports();

    table = dictionary_create(pointer_hash_function, shallow_compare,
                              shallow_copy_constructor,
                              shallow_destructor,
                              shallow_copy_constructor,
                              shallow_destructor);


    pthread_t open_ears_thread;
    if(pthread_create(&open_ears_thread, NULL, open_ears, NULL)){
        perror("pthread_create() error");
        exit(1);
    }

    sleep(10);


    pthread_t seek_out_thread;
    if(pthread_create(&seek_out_thread, NULL, seek_out, NULL)){
        perror("pthread_create() error");
        exit(1);
    }


    pthread_exit(NULL);

    
    return 0;
}




