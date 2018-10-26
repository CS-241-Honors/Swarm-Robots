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
#include "client_node.h"
#include "parse.h"
//-------------------------------------------------------------

client * head;
//-------------------------------------------------------------
static int exit_flag = 0; 
static char server_name[MAX_NAME_LENGTH + 1];

void SIGINT_handler();
void * listen_handler(void * args) {return NULL;}
void * connect_handler(void * args) {return NULL;}
void * receive_handler(void * args) {return NULL;}
void * send_handler(void * args) {return NULL;}
void print_error(char * error){}

//int set_ip_and_port(char * server_ip_arr, int server_ip_len, char * port_arr, int port_len);

//-------------------------------------------------------------

int main(int argc, char ** argv) {
    if (argc < 1) {
        print_error("please enter the number of bot you are");
        printf("correct usage: %s\n", "to do");
        return -1;
    }
    signal(SIGINT, SIGINT_handler);

    // Set up the name
    if (set_name(server_name) == FAILURE_VAL) {
        return -1;
    }

    pthread_t listen_thread;
    pthread_t connect_thread;
    pthread_t receive_thread;
    pthread_t send_thread;

    if (pthread_create(&listen_thread,  NULL, (void *) listen_handler,  NULL)  ||
        pthread_create(&connect_thread, NULL, (void *) connect_handler, NULL) ||
        pthread_create(&receive_thread, NULL, (void *) receive_handler, NULL) ||
        pthread_create(&send_thread,    NULL, (void *) send_handler,    NULL)) {
        print_error("Failed to create threads");
        return -1;
    }
    void * dummy = NULL;
    pthread_join(listen_thread,  &dummy);
    pthread_join(connect_thread, &dummy);
    pthread_join(receive_thread, &dummy);
    pthread_join(send_thread,    &dummy);

    // exit
    //while (!exit_flag) {}
    
    printf("See you, %s\n", server_name);

    //TODO close(network_socket);
    return 0;
}
//-------------------------------------------------------------
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void SIGINT_handler() {
    exit_flag = 1;
}


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void * receive_handler(void * args) {
    char msg[MAX_MESSAGE_LENGTH + 1];
    memset(msg, 0, MAX_MESSAGE_LENGTH + 1);

    



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

/*
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void * send_handler(void * args) {
    int network_socket = (size_t) args;
    char client_msg[MAX_MESSAGE_LENGTH + 1];
    while (!exit_flag) {
        if ( fgets(client_msg, MAX_MESSAGE_LENGTH, stdin) ) {
            if (client_msg[0] == '\n') {
                fprintf(stdout, "%s\n", "The message must have length greater than 0");    
            }
            else {
                fprintf(stdout, "%s: %s\n", client_name, client_msg);
                send(network_socket, client_msg, strlen(client_msg), 0);    
            }
        }    
    }
    return NULL;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void * connect_handler(void * args) {
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

    pthread_t connect_thread;
    // Connect
    int connection_status = connect(network_socket, (struct sockaddr *) & server_address, sizeof(server_address));

    // Print out if connected or not
    if (connection_status == -1) {
        printf("There is an error connecting to the server.\n");
        return FAILURE_VAL;
    }
}
*/
