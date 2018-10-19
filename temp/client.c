#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>
#include <signal.h>
#include <pthread.h>

#include "common.h"

int exit_flag = 0; 

static char client_name[MAX_NAME_LENGTH + 1];
static char server_name[MAX_NAME_LENGTH + 1];

void SIGINT_handler();

void * receive_handler(void * args);

void * send_handler() {
//replace '\n' by '\0' and return the position of '\n'
int remove_next_line(char * input);
int set_client_name();
int set_ip_and_port(char * server_ip_arr, int server_ip_len, char * port_arr, int port_len);


int main() {
    // Exit if CTRL+C
    signal(SIGINT, SIGINT_handler);

    // Set up the name
    if (set_client_name() == FAILURE_VAL) {
        return FAILURE_VAL;
    }

    // Get the IP address and the port the user wants to connect to
    char server_ip[50 + 1];
    char port_str[6 + 1];
    if (set_ip_and_port(server_ip, 50, port_str, 6) == FAILURE_VAL) {
        return FAILURE_VAL;
    }
    char * dummy;
    long int port_num = strtol(port_str, &dummy, 10); 
    
    // Create the socket
    int network_socket;
    network_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons((int) port_num);
    server_address.sin_addr.s_addr = inet_addr(server_ip);

    // Connect
    int connection_status = connect(network_socket, (struct sockaddr *) & server_address, sizeof(server_address));

    // Print out if connected or not
    if (connection_status == -1) {
        printf("There is an error connecting to the server.\n");
        return FAILURE_VAL;
    }


    // pthread receive_handler
    pthread_t receive_thread;
    void * receive_args;
    receive_args = (void *) ((size_t) network_socket);
    if ( pthread_create(&receive_thread, NULL, (void *) receive_handler, receive_args) ) {
        return FAILURE_VAL;
    }

    // pthread send_handler
    pthread_t send_thread;
    if ( pthread_create(&send_thread, NULL, (void *) send_handler, NULL) ) {
        return FAILURE_VAL;
    }

    // exit
    while (!exit_flag) {}
    
    printf("See you, %s\n", client_name);

    close(network_socket);
    return SUCCESS_VAL;
}

//>>>>>
int remove_next_line(char * input) {
    int itr = 0;
    while (!exit_flag) {
        if (input[itr] == '\n') {
            input[itr] = '\0';
            return itr;
        }
        itr++;
    }
    exit(SUCCESS_VAL);
}

//>>>>>
void SIGINT_handler() {
    exit_flag = 1;
}

//>>>>>
int set_client_name() {
    while (!exit_flag) {
        printf("Enter a name with length greater than 0: ");
        if ( fgets(client_name, MAX_NAME_LENGTH, stdin) ) {
            if (client_name[0] == '\n') {
                printf("The length of the name must be greater than 0.\n");
            }
            else {
                remove_next_line(client_name);
                return SUCCESS_VAL;
            }
        }
        else {
            printf("Fail to get the name for some reason.\n");
            return FAILURE_VAL;
        }
    }
    return SUCCESS_VAL;
}

//>>>>>
int set_ip_and_port(char * server_ip_arr, int server_ip_len, char * port_arr, int port_len) {
    while (!exit_flag) {
        printf("Enter the IP address you would like to connect to: ");
        if ( fgets(server_ip_arr, server_ip_len, stdin) ) {
            if (server_ip_arr[0] == '\n') {
                printf("Invalid IP address\n");
            }
            else {
                remove_next_line(server_ip_arr);
                break;
            }
        }
        else {
            printf("An error occurred for some reason.\n");
            return FAILURE_VAL;
        }
    }

    while (!exit_flag) {
        printf("Enter the port you would like to connect to: ");
        if ( fgets(port_arr, port_len, stdin) ) {
            if (server_ip_arr[0] == '\n') {
                printf("Invalid port\n");
            }
            else {
                remove_next_line(port_arr);
                return  SUCCESS_VAL;
            }
        }
        else {
            printf("An error occurred for some reason.\n");
            return FAILURE_VAL;
        }
    }
    return SUCCESS_VAL;
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
        if ( fgets(client_msg, MAX_MESSAGE_LENGTH) ) {
            if (client_msg[0] = '\n') {
                printf("The message must have length greater than 0");    
            }
            else {
                fprintf(stdout, "%s: %s\n", client_name, client_msg);
                send(network_socket, client_msg, strlen(client_msg), 0);    
            }
        }    
    }
    return NULL;
}
