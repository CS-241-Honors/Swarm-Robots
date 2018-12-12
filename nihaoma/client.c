#include "common.h"
#include "dictionary.h"
// #include "format.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wiringPi.h>


#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "pthread.h"
#include "set.h"


//----------------------------------------------------------------------------------
// Definitions
#define LOCAL_IP "127.0.0.1"
#define BACKLOG 16
#define APORT "9000"
#define BPORT "9001"
#define CPORT "9002"
#define DPORT "9003"


//----------------------------------------------------------------------------------
// Global variables
static char my_name;
static char * my_port;
static dictionary * table;
static vector * seek_ports;
static int population_size = 4;
static char population[4] = {'A', 'B', 'C', 'D'};
static int messages_sent; 

static int * my_pins;

static int pinsA[4] = {2, 3, 4, 17};
static int pinsB[4] = {27, 22, 10, 9};
static int pinsC[4] = {11, 5, 6, 13};
static int pinsD[4] = {21, 20, 16, 12};
//----------------------------------------------------------------------------------
// Structures
/**
 * te_metadata = Table Entry Metadata
 * <key, value> == <char, te_metadata>
 */
typedef struct value{
    int direct;
    int fd;
} value;

typedef struct message{
    int id;
    char to;
    char from;
    int type; // 1, que
    int subtype; // 1, 2, 3, 4,
    char request_name;
} message;

enum { 
    QUERY,
    RESPONSE,
    DISCONNECT
};

enum { 
    NO,
    YES
};

//----------------------------------------------------------------------------------

int * get_pins(){
    int * pins;
    switch(my_name){
       
        case 'A':
            
            pins = pinsA;
            break;
        case 'B':
            
            pins = pinsB;
            break;
        case 'C':
            
            pins = pinsC;
            break;
        case 'D':
            
            pins = pinsD;
            break;
    }

    return pins;


}
    

//----------------------------------------------------------------------------------


void turn_light_on(char neighbor_name){
    int index;
     switch(neighbor_name){

        case 'A':
            index = 0;
            break;
        case 'B':
            index = 1;
            break;
        case 'C':
            index = 2;
            break;
        case 'D':
            index = 3;
            break;
    }

//     fprintf(stderr, "delay_len: %f\n", delay_len);
    wiringPiSetupGpio ();
    int LED = my_pins[index];
    pinMode(LED, OUTPUT);

    digitalWrite(LED, 1);
    // delay(delay_len);
    // digitalWrite(LED, 0);
    // delay(delay_len);



}

//----------------------------------------------------------------------------------


void send_to_all_neighbors(void * package){
    (void)package;

    for (int i = 0; i < population_size; i++){
        if(dictionary_contains(table, (void *)(size_t)population[i])){
            
            value * curr_value = dictionary_get(table, (void *)(size_t)(population[i]));
            int curr_fd = curr_value->fd;
            printf("Sending query package to %c on fd #%d\n", population[i], curr_fd);
            write(curr_fd, package, sizeof(message));
        }
    }

    sleep(1);

}

//----------------------------------------------------------------------------------
// int get_fd_from_table(char key){

//     printf("KEY OUT : %c\n", key);
//     value * value_ = (value *) dictionary_get(table, (void *) (size_t) key);

//     return value_->fd;

// }

//----------------------------------------------------------------------------------
/**
 * TODO: implement the return values
 * returns 1 on success
 * returns -1 on failure
 */
int send_message(char to, int type, int subtype, char request_name){
    message * package = (message *) malloc(sizeof(message));
    package->id = messages_sent;
    messages_sent += 1;
    package->to = to;
    package->from = my_name;
    package->type = type;
    package->subtype = subtype;
    package->request_name = request_name;

    // int fd = get_fd_from_table(to);

    // printf("File descriptor recipient : %d\n", fd);
    // (void)fd;
    // char tmp_buf[sizeof(message)];

    // printf("MESSAGE :\n");
    // printf("package->id : %d\n", package->id);
    // printf("package->to : %c\n", package->to);
    // printf("package->from : %c\n", package->from);
    // printf("package->type : %d\n", package->type);
    // printf("package->subtype : %d\n", package->subtype);
    // printf("package->request_name : %d\n", package->request_name);


    // write(fd, package, sizeof(message));
    if (to == 'Z'){
        send_to_all_neighbors(package);
    }else{
        value * curr_value = dictionary_get(table, (void *)(size_t)(to));
        int curr_fd = curr_value->fd;

        printf("Sending a singular message to %c on fd #%d\n", to, curr_fd);
        int write_err = write(curr_fd, package, sizeof(message));
        if (write_err == -1){
            perror("write() error");
            return 0;    
        }
    }

    return 0;

}


//----------------------------------------------------------------------------------
/**
 * read from handler
 * 
 */

void message_interpreter(char messenger_name, int messenger_fd, message * package){

    if (package->type == QUERY){
        printf("We have a query for %c\n", package->request_name);
        char query_name = package->request_name;
        if (dictionary_contains(table, (void *)(size_t)query_name)){ // if we do know this person
            send_message(messenger_name, RESPONSE, YES, package->request_name);
        }else{ // we don't know this person
            send_message(messenger_name, RESPONSE, NO, package->request_name);
        }

    }else if (package->type == RESPONSE){
        printf("Response message received\n");

        if (package->subtype == YES){
            char new_find = package->request_name;
            value * new_value = (value *) malloc(sizeof(value));
            new_value->direct = messenger_name;
            new_value->fd = messenger_fd;

            dictionary_set(table, (void *)(size_t)new_find, new_value);
            turn_light_on(new_find);

        }else{

        }
    }

}

//----------------------------------------------------------------------------------
/**
 * read from handler
 * 
 */

void * read_neighbor_handler(void * name){
    char neighbor_name = (char)(size_t)name;
    printf("Now reading from neighbor %c...\n", neighbor_name);
    value * neighbor_value = (value *) dictionary_get(table, name);
    int fd = neighbor_value->fd;
    while(1){

        // 
        char message_buffer[sizeof(message)];
        printf("Reading from neighbor %c on fd %d...\n", neighbor_name, fd);
        int bytes_read = read(fd, message_buffer, sizeof(message));
        (void)bytes_read;
        printf("Received message from : %c\n", ((message *)message_buffer)->from);

        message * new_package = (message *) message_buffer;

        message_interpreter(neighbor_name, fd, new_package);

        sleep(1);

    }
}

//----------------------------------------------------------------------------------
/**
 * nnwrite == new neighbor write
 * 
 */

void nnwrite(int sock_fd){


    char mesg_buffer[sizeof(message)];
    message * mesg_ = (message *) mesg_buffer;
    mesg_->from = my_name;
    printf("Inside nnwrite(), sending message...\n");
    write(sock_fd, mesg_buffer, sizeof(message));


}

//----------------------------------------------------------------------------------
/**
 * nnread == new neighbor read
 * 
 */

void nnread(int new_neighbor_fd){

    char init_buffer[sizeof(message)];    
    read(new_neighbor_fd, init_buffer, sizeof(message));
    message * message_data = (message *)init_buffer;
    char new_neighbor_name = message_data->from;
    // fill out value
    value * new_value = (value *) malloc(sizeof(value));
    new_value->fd = new_neighbor_fd;
    new_value->direct = 1;
    // throw it in table

    printf("Attempting to add to our dictionary.....\n");
    if(!dictionary_contains(table, (void *)(size_t)new_neighbor_name)){

        dictionary_set(table, (void *)(size_t)new_neighbor_name, (void *)new_value);
        turn_light_on(new_neighbor_name);
        printf("Robot %c is now added to my dictionary and has fd #%d\n", 
                                    new_neighbor_name, new_neighbor_fd);

        pthread_t read_from_thread;
        if(pthread_create(&read_from_thread, NULL, read_neighbor_handler, 
                                    (void *)(size_t)new_neighbor_name)){
            perror("pthread_create() error");
            exit(1);

        }

    }else{
        printf("Not adding to our dictionary, already present\n");
    }
}

//----------------------------------------------------------------------------------
/**
 * populates seek_ports vector with the neighbors to connect to
 * 
 */
void populate_seek_ports(){
    switch(my_name){
        case 'A':
            vector_push_back(seek_ports, BPORT);
            vector_push_back(seek_ports, CPORT);
            break;
        case 'B':
            vector_push_back(seek_ports, APORT);
            vector_push_back(seek_ports, DPORT);
            break;
        case 'C':
            vector_push_back(seek_ports, APORT);
            vector_push_back(seek_ports, DPORT);
            break;
        case 'D':
            vector_push_back(seek_ports, BPORT);
            vector_push_back(seek_ports, CPORT);
            break;
        // default:
            // printf("Did not find appropriate port\n");
            // exit(1);
    }

}




//----------------------------------------------------------------------------------
/**
 * get port
 * 
 */
char * get_port(){
    switch(my_name){
        case 'A':
            my_port = "9000";
            break;
        case 'B':
            my_port = "9001";
            break;
        case 'C':
            my_port = "9002";
            break;
        case 'D':
            my_port = "9003";
            break;
        default:
            printf("Did not find appropriate port\n");
            exit(1);
    }

    return my_port;
}

//----------------------------------------------------------------------------------
/**
 * fill out the rest of the table
 * 
 */

void * table_handler(){

    int counter = 0;
    while(1){
        char current_neighbor = population[counter%population_size];
        if (!dictionary_contains(table, (void *)(size_t)current_neighbor)){
            if (current_neighbor != my_name){
                printf("NEED TO CONNECT TO : %c\n", current_neighbor);
                send_message('Z', QUERY, 0, current_neighbor);
                sleep(1);
                printf("Still filling out the table...\n");
            }
        }
        counter++;
    }



    return NULL;
}





//----------------------------------------------------------------------------------
/**
 * start listening for incoming connections in a loop
 * store any open connections into a table
 */

void * open_ears(void * ignore){
    (void)ignore;

    struct addrinfo hints, * res;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int s = getaddrinfo(NULL, my_port, &hints, &res); 
    if (s != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s)); 
            exit(1);
    }
    
    int ear_fd = socket(AF_INET, SOCK_STREAM, 0);
    int optval = 1;
    setsockopt(ear_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    if (bind(ear_fd, res->ai_addr, res->ai_addrlen) != 0) {
        perror("bind() error"); 
        exit(1);
    }

    if (listen(ear_fd, BACKLOG) != 0) {
        perror("listen() error"); 
        exit(1);
    }

    while(1){
        // accept neighbor
        printf("Now accepting...\n");
        int new_neighbor_fd = accept(ear_fd, NULL, NULL);
        // read the init message
        
        printf("Now reading from new neighbor...\n");

        nnwrite(new_neighbor_fd);
        nnread(new_neighbor_fd);
        
    }


    return NULL;
}

//----------------------------------------------------------------------------------
/**
 * connect to this port and ip
 * 
 */
 void connect_to(char * host, char * port){
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;                            
    hints.ai_socktype = SOCK_STREAM;


    int s = getaddrinfo(host, port, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    }

    int sock_fd = socket(hints.ai_family, hints.ai_socktype, 0);
    if (sock_fd == -1) {
        fprintf(stderr, "client: fail to create a socket.\n");
    }

    while(1){

        int ok = connect(sock_fd, result->ai_addr, result->ai_addrlen);
        if (ok == -1) {
            fprintf(stderr, "client: fail to connect.\n");
        }else{
            break;
        }

        sleep(1);

    }



    nnwrite(sock_fd);
    nnread(sock_fd);



    return;
 }

//----------------------------------------------------------------------------------
/**
 * seek out pre-defined neighbors
 * 
 */

void * seek_out(void * ignore){
    (void)ignore;


    for(size_t i = 0; i < vector_size(seek_ports); i++){
        connect_to(LOCAL_IP, vector_get(seek_ports, i));
        
    }

    printf("Done seeking pre-defined neighbors\n");


    return NULL;
}


//----------------------------------------------------------------------------------
/**
 * main function
 * 
 */

void sigpipe_handler(){
    printf("SIGPIPE received\n");
}

int main(int argc, char * argv[]){

    signal(SIGPIPE, sigpipe_handler);

    (void)argc;
    my_name =  argv[1][0];
    printf("My name is %c\n", my_name);
    my_port = get_port();
    printf("My port is %s\n", my_port);

    seek_ports = vector_create(string_copy_constructor,
                      string_destructor,
                      string_default_constructor);

    populate_seek_ports();

    my_pins = get_pins();

    table = dictionary_create(pointer_hash_function, shallow_compare,
                              shallow_copy_constructor,
                              shallow_destructor,
                              shallow_copy_constructor,
                              shallow_destructor);


    pthread_t open_ears_thread;
    if(pthread_create(&open_ears_thread, NULL, open_ears, NULL)){
        perror("pthread_create() error");
        exit(1);
    }

    sleep(5);


    pthread_t seek_out_thread;
    if(pthread_create(&seek_out_thread, NULL, seek_out, NULL)){
        perror("pthread_create() error");
        exit(1);
    }

    sleep(1);

    pthread_t table_thread;
     if(pthread_create(&table_thread, NULL, table_handler, NULL)){
        perror("pthread_create() error");
        exit(1);
    }



    printf("pthread_exit-ing...\n");
    pthread_exit(NULL);

    
    return 0;
}


//----------------------------------------------------------------------------------
/**
 *  EXTRA
 * 
 */

// while(1){
        //     printf("Reading from neigbor %c\n", neighbor_name);
        //     sleep(2);
        // }

