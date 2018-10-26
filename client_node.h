#include <string.h>
#include "common.h"

typedef struct client {
    int socket;
    char ip[50 + 1]; //maximum 50 bytes allowed
    long int port;
    char name[MAX_NAME_LENGTH + 1];
    struct client * next;
} client;

//return 0 upon success, -1 otherwise
int insert_client(client ** head, client * new_node);
int create_client(client ** head, char * ip, long int port, char * client_name);
void delete_client(client ** head, char * ip, long int port);
void delete_all_client(client ** head);

