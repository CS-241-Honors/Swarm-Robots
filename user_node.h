#include <assert.h>
#include <string.h>
#include "common.h"

typedef struct user_info {
    int socket;
    char ip[50 + 1]; //maximum 50 bytes allowed
    long int port;
    char name[MAX_NAME_LENGTH + 1];
    struct user_info * next;
} user_info;

// next value is set to NULL
user_info * create_user_info(int socket, char * ip, long int port, char * user_info_name);

// user_info ** so that we can directly modify the head pointer
void insert_user_info(user_info ** head_addr, user_info * new_node);
// return NULL if not found
user_info * find_user_info(user_info ** head_addr, char * ip, long int port);
void delete_user_info(user_info ** head_addr, char * ip, long int port);
void delete_all_user_info(user_info ** head_addr);

//-------------------------------------------------------------

user_info * create_user_info(int socket, char * ip, long int port, char * user_info_name) {
    assert(strlen(ip) <= 50);
    assert(strlen(user_info_name) <= MAX_NAME_LENGTH);

    user_info * rv = malloc(sizeof(user_info));
    rv->socket = socket;
    memcpy(rv->ip, ip, strlen(ip) + 1);
    rv->port = port;
    memcpy(rv->name, user_info_name, strlen(user_info_name) + 1);
    rv->next = NULL;
    return rv;
}

void insert_user_info(user_info ** head_addr, user_info * new_node) {
    user_info * head = *head_addr;
    if (head == NULL) {
        *head_addr = new_node;
    }
    else {
        new_node->next = head->next;
        *head_addr = new_node;
    }
}

user_info * find_user_info(user_info** head_addr, char* ip, long int port) {
    user_info * head = *head_addr;

    while (head) {
        if (head->ip == ip && head->port == port) {
            return head;
        }
        head = head->next;
    }
    return NULL;
}

void delete_user_info(user_info ** head_addr, char * ip, long int port) {
    user_info * head = *head_addr;
    if (head == NULL) {
        return;
    }
    else if (head->ip == ip && head->port == port) {
        user_info * temp = head;
        *head_addr = head->next;
        free(temp); temp = NULL;
    }
    else {
        user_info * curr = head;
        user_info * next = head->next;
        while (next) {
            if (next->ip == ip && next->port == port) {
                user_info * temp = next;
                curr->next = next->next;
                free(temp); temp = NULL;
                return;
            }
            curr = next;
            next = curr->next;
        }
    }
}

void delete_all_user_info(user_info ** head_addr) {
    user_info * head = NULL;
    while (head) {
        user_info * temp = head->next;
        free(head); head = NULL;
        head = temp;
    }
}
~             
