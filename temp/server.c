//bind function specifies what IP & port address the servers listens to 
//socket -> bind -> listen -> accept

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <string.h>
#include <signal.h>
#include <pthread.h>

#include "common.h"
//>>>
static int exit_flag = 0;

void SIGINT_handler();
void * receive_handler();
void * send_handler();
//>>>
int main() {
    
    char server_message[256] = "You have reached the server!";

    //create the server socket
    int server_socket;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // define the server_address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(9002);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // bind the socket to our specified IP and prot
    bind(server_socket, (struct sockaddr *) & server_address, sizeof(server_address));

    //second parameter indicates how many client the server can deal with at a time
    listen(server_socket, 5);

    int client_socket;
    //second para will be filled with the address
    //third is the size
    client_socket = accept(server_socket, NULL, NULL);

    //send the message
    //first the socket we want to send the data on
    send(client_socket, server_message, sizeof(server_message), 0);

    pthread_t receive_thread;
    void * receive_args = (void *) ((size_t) client_socket);
    if ( pthread_create(&receive_thread, NULL, (void *) receive_handler, receive_args) ) {
        return FAILURE_VAL;
    }

    puts("61");
    pthread_t send_thread;
    void * send_args = (void *) ((size_t) client_socket); 
    if ( pthread_create(&send_thread, NULL, (void *) send_handler, send_args) ) {
        return FAILURE_VAL;
    }

    while (!exit_flag) {} 
    //close the socket
    close(server_socket);



    return 0;    
}

//>>>>>
void SIGINT_handler() {
    exit_flag = 1; 
}

//>>>>>
void * receive_handler(void * args) {
    int network_socket = (size_t) args;
    char server_response[MAX_MESSAGE_LENGTH + 1];
    server_response[0] = '\0';
    while (!exit_flag) {
        int recv_status = recv(network_socket, & server_response, sizeof(server_response), 0);
        if (recv_status > 0) {
            fprintf(stdout, "Server: %s\n", server_response);
            //should be improved later
            memset(server_response, 0, MAX_MESSAGE_LENGTH);
        }
    }
    return NULL;
}

//>>>>>
void * send_handler(void * args) {
    int network_socket = (size_t) args;
    char client_msg[MAX_MESSAGE_LENGTH + 1];
    while (!exit_flag) {
        if ( fgets(client_msg, MAX_MESSAGE_LENGTH, stdin) ) {
            if (client_msg[0] == '\n') {
                fprintf(stdout, "%s\n", "The message must have length greater than 0");
            }
            else {
                fprintf(stdout, "Server: %s\n", client_msg);
                send(network_socket, client_msg, strlen(client_msg), 0);
            }
        }
    }
    return NULL;
}
