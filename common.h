#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>
#include <signal.h>
#include <pthread.h>

//macros
#define MAX_NAME_LENGTH 100
#define MAX_MESSAGE_LENGTH 1000

#define PORT1 5000
#define PORT2 5001
#define PORT3 5002
#define PORT4 5003

#define LOCAL_IP "127.0.0.1"
