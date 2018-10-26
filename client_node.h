#include <assert.h>
#include <string.h>
#include "common.h"

typedef struct client {
    int socket;
    char ip[50 + 1]; //maximum 50 bytes allowed
    long int port;
    char name[MAX_NAME_LENGTH + 1];
    struct client * next;
} client;

// next value is set to NULL
client * create_client(int socket, char * ip, long int port, char * client_name);

// client ** so that we can directly modify the head pointer
void insert_client(client ** head_addr, client * new_node);
// return NULL if not found
client * find_client(client ** head_addr, char * ip, long int port);
void delete_client(client ** head_addr, char * ip, long int port);
void delete_all_client(client ** head_addr);

//-------------------------------------------------------------

client * create_client(int socket, char * ip, long int port, char * client_name) {
    assert(strlen(ip) <= 50);
    assert(strlen(client_name) <= MAX_NAME_LENGTH);

    client * rv = malloc(sizeof(client));
    rv->socket = socket;
    memcpy(rv->ip, ip, strlen(ip) + 1);
    rv->port = port;
    memcpy(rv->name, client_name, strlen(client_name) + 1);
    rv->next = NULL;
    return rv;
}

void insert_client(client ** head_addr, client * new_node) {
    client * head = *head_addr;
    if (head == NULL) {
        *head_addr = new_node;
    }
    else {
        new_node->next = head->next;
        *head_addr = new_node;
    }
}

client * find_client(client** head_addr, char* ip, long int port) {
    client * head = *head_addr;

    while (head) {
        if (head->ip == ip && head->port == port) {
            return head;
        }
        head = head->next;
    }
    return NULL;
}

void delete_client(client ** head_addr, char * ip, long int port) {
    client * head = *head_addr;
    if (head == NULL) {
        return;
    }
    else if (head->ip == ip && head->port == port) {
        client * temp = head;
        *head_addr = head->next;
        free(temp); temp = NULL;
    }
    else {
        client * curr = head;
        client * next = head->next;
        while (next) {
            if (next->ip == ip && next->port == port) {
                client * temp = next;
                curr->next = next->next;
                free(temp); temp = NULL;
                return;
            }
            curr = next;
            next = curr->next;
        }
    }
}

void delete_all_client(client ** head_addr) {
    client * head = NULL;
    while (head) {
        client * temp = head->next;
        free(head); head = NULL;
        head = temp;
    }
}
