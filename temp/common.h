#include <stdlib.h>
#include <stdio.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/in.h>

#define MAX_NAME_LENGTH 100;
#define MIN_NAME_LENGTH 1;

int FAILURE_VAL = -1;
int SUCCESS_VAL = 0;
