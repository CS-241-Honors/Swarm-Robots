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
#include "user_node.h"
#include "parse.h"

//-------------------------------------------------------------
static pthread_rwlock_t rwlock;
static int exit_flag = 0; 
static char this_user_name[MAX_NAME_LENGTH + 1];
static user_info * other_user1;
static user_info * other_user2;
static user_info * other_user3;
static user_info * all_other_users = NULL; // linked list

//-------------------------------------------------------------
void SIGINT_handler();
void sent_msg_to_all(char * msg);
void * recv_handler(void * args);
void * send_handler(void * args);

//-------------------------------------------------------------
int main(int argc, char ** argv) {
    if (argc < 1) {
        fprintf(stderr, "please specify which bot you are (1, 2, 3, or 4");
        printf("correct usage: %s\n", "number");
        return -1;
    }
    if (set_name(this_user__name) == FAILURE_VAL) {
        puts("Failed to create the name");
        return -1;
    }
    pthread_rwlock_init(&rwlock, NULL);
    signal(SIGINT, SIGINT_handler);
    //-------------------------------------------------
    int bot_num = atoi(argv[0]);
    int connect_status = connect_to_others(bot_num);
    if (connect_status != 0) {
        puts("Failed to connect");
        return -1;
    }
    //-------------------------------------------------
    pthread_t recv_thread1;
    pthread_t recv_thread2;
    pthread_t recv_thread3;
    pthread_t send_thread;
    if (pthread_create(&recv_thread1, NULL, recv_handler, (void *) other_user1) ||
        pthread_create(&recv_thread2, NULL, recv_handler, (void *) other_user2) ||
        pthread_create(&recv_thread3, NULL, recv_handler, (void *) other_user3) ||
        pthread_create(&send_thread,  NULL, send_handler, NULL)) {
        fprintf(stderr, "Failed to create threads.\n"); 
        return -1;
    }
    void * dummy = NULL;
    pthread_join(recv_thread1, &dummy);
    pthread_join(recv_thread2, &dummy);
    pthread_join(recv_thread3, &dummy);
    pthread_join(send_thread, &dummy);
    //-------------------------------------------------
    printf("See you, %s\n", server_name);
    pthread_rwlock_destroy(&rwlock);
    return 0;
}
//-------------------------------------------------------------

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void SIGINT_handler() {
    exit_flag = 1;
    //TODO close(network_socket);
    pthread_rwlock_wrlock(&rwlock); 
}


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void sent_msg_to_all(char * msg) {
    size_t msg_len = sizeof(msg);
    if (msg_len == 0) {
        return;    
    }
    pthread_rwlock_rdlock(&rwlock); 
    user_info * itr = all_other_users;
    while (itr) {
        int network_socket = itr->socket; 
        send(network_socket, msg, msg_len, 0); 
        itr = itr->next;
    }
    pthread_rwlock_unlock(&rwlock);
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void * recv_handler(void * _other_user) { 
    user_info * other_user = (user_info * ) _other_user;
    char msg[MAX_MESSAGE_LENGTH + 1];
    memset(msg, 0, MAX_MESSAGE_LENGTH + 1);

    while (!exit_flag) {
        int recv_status = recv(other_user->socket, &msg, 1000, 0);
        if (recv_status > 0) {
            //the client exited 
            if ( !strcmp(msg, EXIT_MSG) ) {
                pthread_rwlock_wrlock(&rwlock); 
                delete_user_info(&all_other_users, other_user->ip, other_user->port);
                pthread_rwlock_unlock(&rwlock);
                return NULL;
            }    
            // check if the message if empty
            else if ( msg[0] != '\0' ) {
                printf("%s: %s\n", other_user->name, msg);
                memset(msg, 0, MAX_MESSAGE_LENGTH + 1);
            }
        }
    }
    pthread_rwlock_wrlock(&rwlock);
    close(other_user->socket);
    delete_user_info(other->ip, other->port);
    pthread_rwlock_unlock(&rwlock);
    return NULL;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void * send_handler(void * dummy) {
    (void) dummy;
    size_t total_len = strlen(this_user_name) + 2 + MAX_MESSAGE_LENGTH + 1;
    char name_and_msg[total_len];
    strcpy(name_and_msg, this_user_name);
    strcpy(name_and_msg + strlen(this_user_name), ": ");
    char * msg = name_and_msg + strlen(this_user_name) + 2;
    memcpy(msg, 0, MAX_MESSAGE_LENGTH + 1);
    
    while (!exit_flag) {
        if ( fgets(msg, MAX_MESSAGE_LENGTH, stdin) ) {
            if (msg[0] == '\n') {
                fprintf(stderr, "%s\n", "The message must have length greater than 0");    
            }
            else {
                remove_next_line(msg);
                send_msg_to_all(msg);    
            }
        }    
        msg[0] = '\0';
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

