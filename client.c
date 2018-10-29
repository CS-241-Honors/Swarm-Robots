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
//static user_info * other_user3;
static user_info * all_other_users = NULL; // linked list

//-------------------------------------------------------------
void SIGINT_handler();
void send_msg_to_all(char * msg);
void * recv_handler(void * args);
void * send_handler(void * args);

int connect_to_others(int bot_num);
//-------------------------------------------------------------
int main(int argc, char ** argv) {
    if (argc < 1) {
        fprintf(stderr, "please specify which bot you are (1, 2, or3");
        printf("correct usage: %s\n", "number");
        return -1;
    }
    if (set_name(this_user_name) == -1) {
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
    pthread_t send_thread;
    if (pthread_create(&recv_thread1, NULL, recv_handler, (void *) other_user1) ||
        pthread_create(&recv_thread2, NULL, recv_handler, (void *) other_user2) ||
        pthread_create(&send_thread,  NULL, send_handler, NULL)) {
        fprintf(stderr, "Failed to create threads.\n"); 
        return -1;
    }
    void * dummy = NULL;
    pthread_join(recv_thread1, &dummy);
    pthread_join(recv_thread2, &dummy);
    pthread_join(send_thread, &dummy);
    //-------------------------------------------------
    printf("See you, %s\n", this_user_name);
    pthread_rwlock_destroy(&rwlock);
    return 0;
}
//-------------------------------------------------------------

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void SIGINT_handler() {
    exit_flag = 1;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void send_msg_to_all(char * msg) {
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
    delete_user_info(&all_other_users, other_user->ip, other_user->port);
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
/*
void default_address_setup(struct sock_addr_in * address, char * ip, long int port) {
    server_address->sin_family = AF_INET;
    server_address->sin_port = htons((int) port);
    server_address->sin_addr.s_addr = inet_addr(ip);
}

// 1 connects to 2, 3, 4; 1 receives 
// 2 connects to    3, 4; 2 receives 1
// 3 connects to       4; 3 receives 1, 2
// 4 connects to        ; 1 receives 1, 2, 3


int connect_helper(long int port) {
    int socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons((int) port);
    address.sin_addr.s_addr = inet_addr(ip);
        
        
        
        default_address_setup(&address2, LOCAL_IP, PORT2);
        int status2 = connect(socket2, (struct sockaddr *) & server_address, sizeof(address2));
        if (status2 == -1) {
            return -1;    
        }
    
}

int receive_helper(int from) {
    
}

int connect_handler(int bot_num) {
    if (num == 1) {
        int socket2 = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in address2;
        default_address_setup(&address2, LOCAL_IP, PORT2);
        int status2 = connect(socket2, (struct sockaddr *) & server_address, sizeof(address2));
        if (status2 == -1) {
            return -1;    
        }
        int socket3 = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in address3;
        default_address_setup(&address3, LOCAL_IP, PORT3);
        int status3 = connect(socket3, (struct sockaddr *) & server_address, sizeof(address3));
        if (status3 == -1) {
            return -1;    
        }
        int socket4 = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in address4;
        default_address_setup(&address4, LOCAL_IP, PORT4);
        int status4 = connect(socket4, (struct sockaddr *) & server_address, sizeof(address4));
        if (status4 == -1) {
            return -1;     
        }
        else {
            
        }
    }
    else if (num == 2) {
        int socket3 = socket(AF_INET, SOCK_STREAM, 0);
        int socket4 = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in address3;
        struct sockaddr_in address4;
        default_address_setup(&address2, LOCAL_IP, PORT2);
        default_address_setup(&address3, LOCAL_IP, PORT3);
        default_address_setup(&address4, LOCAL_IP, PORT4);
        int status2 = connect(socket2, (struct sockaddr *) & server_address, sizeof(address2));
        int status3 = connect(socket3, (struct sockaddr *) & server_address, sizeof(address3));
        int status4 = connect(socket4, (struct sockaddr *) & server_address, sizeof(address4));
        if (status2 == -1 || status3 == -1 || status4 == -1) {
            return -1;     
        }
        
    }
    else if (num == 3) {
        
    }
    else if (num == 4) {
        
    }
    else {
        puts("Incorrect argument! Make sure the number passed in is between 1 and 4");
        exit(1);
    }
    return 0;
}
*/
int connect_to_others(bot_num) {
    return 0;    
}

