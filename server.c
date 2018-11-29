//bind function specifies what IP & port address the servers listens to 
//socket -> bind -> listen -> accept

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/in.h>

int main() {
    
    char server_message[256] = "You have reached the server!";

    //create the server socket
    int server_socket;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // define the server_address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(9002);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // bind the socket to our specified IP and prot
    bind(server_socket, (struct sockaddr *) & server_address, sizeof(server_address));

    //second parameter indicates how many client the server can deal with at a time
    listen(server_socket, 5);

    int client_socket;
    //second para will be filled with the address
    //third is the size
    client_socket = accept(server_socket, NULL, NULL);

    //send the message
    //first the socket we want to send the data on
    send(client_socket, server_message, sizeof(server_message), 0);

//    while (1) {
//        pthread_create(send_message, );
//        pthread_create(receive_message, );
//    }


    //close the socket
    close(server_socket);



    return 0;    
}
