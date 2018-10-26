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
#define FAILURE_VAL (-1)
#define SUCCESS_VAL 0

#define port1 5000
#define port2 5001
#define port3 5002
#define port4 5003
