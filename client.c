//From UIUC CS 241 by Angranve Lawrence
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

int main(int argc, char ** argv) {
    int s;
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct addrinfo hints, * result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    s = getaddrinfo("fa18-cs241-330.cs.illinois.edu", "80", &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));    
        exit(1);
    }
    connect(sock_fd, result->ai_addr, result->ai_addrlen);
    char * request = "GET / HTTP/1.0\r\n\r\n";
    puts(request);
    write( sock_fd, request, strlen(request) );

    char response[1000];
    int len = read( sock_fd, response, 999);
    if (len > 0) {
        response[len] = '\0';
        puts(response);
    }
    return 0;
}
