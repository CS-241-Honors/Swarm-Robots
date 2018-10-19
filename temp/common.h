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

#define MAX_NAME_LENGTH 100
#define MAX_MESSAGE_LENGTH 1000
#define FAILURE_VAL (-1)
#define SUCCESS_VAL 0


