#include "client_node.h"

client * create_client(int socket, char * ip, long int port, char * client_name) {
    client * rv = malloc(sizeof(client));
    rv->socket = socket;
    rv->ip = ip;
    rv->port = port;
    rv->name = client_name;
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
    client * head;
    while (head) {
        client * temp = head->next;
        free(head); head = NULL;
        head = temp;
    } 
}
