//type and protol needed to create the socket
//specify the IP and the port to connect
//return value that indicates if the connection is successful
//recv() gives the data from the other end

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

int main() {
    //create a socket
    int network_socket;
    //first para is the domain of the socket
    //second is the type of the socket: TCP / UDP
    //Third is the protocol; 0 means the default protocol of TCP
    network_socket = socket(AF_INET, SOCK_STREAM, 0);

    //specify the address structure 
    struct sockaddr_in server_address;
    // type of the address
    server_address.sin_family = AF_INET;     
    //port we want to connect to 
    //htons convert the actual port number to one recognized by the function
    server_address.sin_port = htons(9002); 
    //specify the actual IP address

    //sin_addr is a structure 
    //sin_addr.s_addr is the real address
    //INADDR_ANY is the local address
    server_address.sin_addr.s_addr = INADDR_ANY;

    //first para is the socket
    //third is the size of the address
    //returns an int to indicate if the connection is successful or not
    //0 means successful, -1 means failed
    int connection_status = connect(network_socket, (struct sockaddr *) & server_address, sizeof(server_address) );
    //check for error with the connection
    if (connection_status == -1) {
        printf("There was an error making a a connnection ot the remote socket \n \n");    
    }

    //once connected, we either send or receives data
    //receive data from the server
    //first para is the socket
    //create a string to hold the information getting back from the server
    char server_response[256];
    recv(network_socket, & server_response, sizeof(server_response), 0);

    // print out the server's reponse
    printf("The server sent the data: %s\n", server_response);

    // close the socket
    close(network_socket);
    return 0;    
}
