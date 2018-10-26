#include <string.h>
#include "common.h"

typedef struct client {
    int socket;
    char ip[50 + 1]; //maximum 50 bytes allowed
    long int port;
    char name[MAX_NAME_LENGTH + 1];
    struct client * next;
} client;

void insert_client(client ** head_addr, client * new_node);
client * create_client(client ** head_addr, char * ip, long int port, char * client_name);

// ** so that we can directly modify the head pointer
void delete_client(client ** head_addr, char * ip, long int port);
void delete_all_client(client ** head_addr);

