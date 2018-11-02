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
static user_info * other_user1 = NULL;
static user_info * other_user2 = NULL;
//static user_info * user3 = NULL;
static user_info * all_other_users = NULL; // linked list

//-------------------------------------------------------------
void SIGINT_handler();
void send_msg_to_all(char * msg);
void * recv_handler(void * args);
void * send_handler(void * args);

void * listen_handler(void * bot_num);
void * connect_handler(void * bot_num);
//-------------------------------------------------------------
int main(int argc, char ** argv) {
    if (argc < 1) {
        fprintf(stderr, "please specify which bot you are (1, 2, or3");
        printf("correct usage: %s\n", "number");
        return -1;
    }
    (void) this_user_name;
    /*
    if (set_name(this_user_name) == -1) {
        puts("Failed to create the name");
        return -1;
    }
    */
    pthread_rwlock_init(&rwlock, NULL);
    signal(SIGINT, SIGINT_handler);
    //-------------------------------------------------
    pthread_t connect_thread;
    pthread_t listen_thread;
    if (pthread_create(&connect_thread, NULL, connect_handler, (void*) argv[1]) ||
        pthread_create(&listen_thread, NULL, listen_handler, (void *) argv[1])) {
        puts("Failed to connect");
        return -1;    
    }
    void * dummy = NULL;
    pthread_join(connect_thread, &dummy);
    pthread_join(listen_thread, &dummy);
    puts("Successfully connected");
    //-------------------------------------------------
    while(1) {}
    pthread_t recv_thread1;
    pthread_t recv_thread2;
    pthread_t send_thread;
    if (pthread_create(&recv_thread1, NULL, recv_handler, (void *) other_user1) ||
        pthread_create(&recv_thread2, NULL, recv_handler, (void *) other_user2) ||
        pthread_create(&send_thread,  NULL, send_handler, NULL)) {
        fprintf(stderr, "Failed to create threads.\n"); 
        return -1;
    }
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

void default_address_setup(struct sockaddr_in * address, char * ip, long int port) {
    address->sin_family = AF_INET;
    address->sin_port = htons((int) port);
    address->sin_addr.s_addr = inet_addr(ip);
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void * connect_handler(void * _bot_num) {
    printf("connect_handler, 165:  %s\n", (char *) _bot_num);
    _bot_num = (char *) _bot_num;
    int bot_num = 0;
    sscanf((char *) _bot_num, "%d", &bot_num);
    int other_bot_num = (bot_num + 1) % 3;
    char other_name[10];
    memset(other_name, 0, 10);
    memcpy(other_name, "Bot ", 4);
    sprintf(other_name + 4, "%d", other_bot_num);

    long int other_port = 5000 + other_bot_num; //hardcoded

    int other_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    default_address_setup(&address, LOCAL_IP, other_port);
    int status = connect(other_sock, (struct sockaddr *) & address, sizeof(address));
    (void) status; 

    user_info * user = create_user_info(other_sock, LOCAL_IP, other_port, other_name); 

    other_user1 = user;

    return NULL;
}
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void * listen_handler(void * _bot_num) {
    _bot_num = (char *) _bot_num;
    int bot_num = 0;
    sscanf((char *) _bot_num, "%d", &bot_num);
    int other_bot_num = (bot_num + 2) % 3;
    char other_name[10];
    memset(other_name, 0, 10);
    memcpy(other_name, "Bot ", 4);
    sprintf(other_name + 4, "%d", other_bot_num);
    fprintf(stderr, "curr_bot_num: %s\n", _bot_num); 
    fprintf(stderr, "other_bot_num: %s\n", other_name); 

    long int port = 5000 + bot_num;
    int other_port = 5000 + (bot_num + 2) % 3;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    default_address_setup(&address, LOCAL_IP, port);
    bind(sock, (struct sockaddr *) & address, sizeof(address));
    listen(sock, 5);

    int other_sock;
    //second para will be filled with the address
    //third is the size
    other_sock = accept(sock, NULL, NULL);
    printf("%s successfully listens\n", (char *) _bot_num);
    user_info * user = create_user_info(other_sock, LOCAL_IP, other_port, other_name); 
    other_user2 = user;
    return NULL; 
}
