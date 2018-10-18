#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include "common.h"

int exit_flag = 0; 

static char client_name[MAX_NAME_LENGTH + 1];
static char server_name[MAX_NAME_LENGTH + 1];

int SIGINT_handler() {
    exit_flag = 1;
}

void * receive_handler() {

    return NULL;
}

void * send_handler() {

    return NULL;
}

void remove_next_line(char * input) {
    int itr = 0;
    while (1) {
        if (input[itr] == '\n') {
            input[itr] = '\0';
            return;
        }
    }
}

int set_name() {
    while (1) {
        printf("Enter a name with length greater than 0: ");
        if ( fgets(client_name, MAX_NAME_LENGTH, stdin) ) {
            if (client_name[0] == '\n') {
                printf("The length of the name must be greater than 0.\n");
            }
            else {
                remove_next_line(client_name);
                exit(SUCCESS_VAL);
            }
        }
        else {
            printf("Fail to get the name for some reason.\n");
            exit(FAILURE_VAL);
        }
    }
    EXIT(SUCCESS_VAL);
}

int set_ip_and_port(char * server_ip_arr, int server_ip_len, char * port_arr, int port_len) {
    while (1) {
        printf("Enter the IP address you would like to connect to: ");
        if ( fgets(server_ip_arr, server_ip_len, stdin) ) {
            if (server_ip_arr[0] == '\n') {
                printf("Invalid IP address\n");
            }
            else {
                remove_next_line(server_ip_arr);
                exit(SUCCESS_VAL);
            }
        }
        else {
            printf("An error occurred for some reason.\n");
            exit(FAILURE_VAL);
        }
    }

    while (1) {
        printf("Enter the port you would like to connect to: ");
        if ( fgets(port_arr, port_len, stdin) ) {
            if (server_ip_arr[0] == '\n') {
                printf("Invalid port\n");
            }
            else {
                remove_next_line(port_arr);
                exit(SUCCESS_VAL);
            }
        }
        else {
            printf("An error occurred for some reason.\n");
            exit(FAILURE_VAL);
        }
    }
    exit(SUCCESS_VAL);
}


int main() {
    // Exit if CTRL+C
    SIGINT_handler(SIGINT, SIGINT_handler);

    // Set up the name
    if (set_name() == FAILURE_VAL) {
        exit(FAILURE_VAL);
    }

    // Get the IP address and the port the user wants to connect to
    char server_ip[50 + 1];
    char port_str[6 + 1];
    if (set_ip_and_port(server_ip, 50, port_str, 6) == FAILURE_VAL) {
        exit(FAILURE_VAL);
    }
    char * dummy;
    long int port_num = strtol(port_str, dummy, 10); 
    
    // Create the socket
    int network_socket;
    network_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons((int) port_num);
    server_address.sin_addr.s_addr = server_ip;

    // Connect
    int connection_status = connect(network_socket, (struct sockaddr *) & server_address, sizeof(server_address));

    // Print out if connected or not
    if (connection_status == -1) {
        printf("There is an error connecting to the server.\n");
        exit(FAILURE_VAL);
    }


    // pthread receive_handler
    pthread_t receive_thread;
    if ( pthread_create(&receive_thread, NULL, (void *) receive_handler, NULL) ) {
        exit(FAILURE_VAL);
    }

    // pthread send_handler
    pthread_t send_thread
    if ( pthread_create(&send_thread, NULL, (void *) send_handler, NULL) ) {
        exit(FAILURE_VAL);
    }

    // exit
    while (1) {
        if (exit_flag) {
            break;
        }
    }
    
    printf("See you, %s\n", client_name);
    close(network_socket);
    exit(SUCCESS_VAL);
}

