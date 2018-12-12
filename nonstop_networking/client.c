/**   
* Networking Lab
* CS 241 - Fall 2018
*/
//--------------------------------------------------------------
#include "includes/dictionary.h"
#include "format.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
#include <wiringPi.h>
//--------------------------------------------------------------
// Definitions
#define PORT1 "5000"
#define PORT2 "5001"
#define PORT3 "5002"
#define PORT4 "5003"
#define LOCAL_IP "127.0.0.1"
#define NEIGHBOR_NUM 2
#define TOTAL_BOT_NUM 4

typedef enum { 
    QUERY,
    RESPONSE,
    MOVE,
    DISCONNECT,
    UNKNOWN
} verb;

typedef struct message{
    int msg_id;
    char from;
    char to;
    verb request;
    char query_bot;
    int direction;
    int distance;
} message; 

//--------------------------------------------------------------
// Global Variables
static char all_bot_names[TOTAL_BOT_NUM] = {'A', 'B', 'C', 'D'};
static dictionary * table;
static char * bot_name;
static char ** neighbor_names;
static int msg_id;
static pthread_mutex_t m;
static int A_leds[4] = {2,   3,  4, 17};
static int B_leds[4] = {27, 22, 10,  9};
static int C_leds[4] = {11,  5,  6, 13};
static int D_leds[4] = {12, 21, 20, 16};
static bool running;
//--------------------------------------------------------------
// Function headers
void print_usage();
void print_invalid_msg_format();
void SIGINT_handler();
void SIGPIPE_handler();

int connect_handler_helper(char * host, char * port);
void * connect_handler(void * dummy);
void * listen_handler(void * dummy);
void do_query(char curr_bot);
void send_query(char curr_bot, char query_bot);
void print_dictionary(dictionary * table);

void send_name(int to_fd, char * name);
char * read_name(int from_fd);
ssize_t read_all_from_fd(int fd, char *buffer, size_t count);
ssize_t write_all_to_fd(int fd, const char *buffer, size_t count);
char * msg_before_next(char * msg);

void clear_all_table_elems();
void clear_neighbor_names(char ** _neighbor_names);

char * verb_to_string(verb v);
verb string_to_verb(char * s);

char * shell_getline();
char ** shell_split_line(char * line);
void free_argv(char *** argv);
int count_argv(char ** argv);
void print_argv(char ** argv);
void * blink_handler(void * _name);

int led_lookup(char curr_name, char other_name);
void turn_light_on(char curr_name, char other_name);
void turn_light_off(char curr_name, char other_name);
void turn_all_lights_off(char name);
//--------------------------------------------------------------
int main(int argc, char ** argv) {
    (void) all_bot_names;
    fprintf(stderr, "%zu\n", sizeof(verb));
    if (argc != 2) {
        print_usage();    
		exit(1);
    } 
	//-----	
    // initialize global variables
	signal(SIGINT, SIGINT_handler);
	signal(SIGPIPE, SIGPIPE_handler);	
    table = dictionary_create(shallow_hash_function, shallow_compare, NULL, NULL, NULL, NULL);
    msg_id = 0;
    bot_name = argv[1];
    neighbor_names = calloc(sizeof(char *), NEIGHBOR_NUM);
    pthread_mutex_init(&m, NULL);
    running = 1; 
	//-----	
    // blink
    pthread_t blink_thread;
    if (pthread_create(&blink_thread, NULL, blink_handler, (void *) (size_t) argv[1][0])) {
        perror("pthread_create");
        exit(1);
    }
	//-----	
    // Connect
    pthread_t connect_thread;
    pthread_t listen_thread;

	if (pthread_create(&connect_thread, NULL, connect_handler, (void*) argv[1]) ||
        pthread_create(&listen_thread, NULL, listen_handler, (void *) argv[1])) {
        fprintf(stderr, "Bot%s fails to join the network.\n", argv[1]);
		exit(1);
    }
    void * dummy = NULL;
    pthread_join(connect_thread, &dummy);
	fprintf(stderr, "Bot%s successfully joins the network.\n", argv[1]);
	//-----	
    // fill out the routing table
    print_dictionary(table);
//    do_query(argv[1][0]);
	//-----	
    // read message
    while (1) {
		fprintf(stderr, "Reading command.\n");
        char * curr_line = shell_getline();
        fprintf(stderr, "after shell_getline\n");
        if (curr_line == NULL) {
            break;    
        }
        /*
		char ** line_argv = shell_split_line(curr_line);
        fprintf(stderr, "after shell_split_line");
    	int line_argc = count_argv(line_argv);
        (void) line_argc;


        for (int i = 0; i < line_argc; i++) {
            fprintf(stderr, "%d: %s\n", i, argv[i]);
        }
		free(curr_line); 
        puts("162");
		print_argv(line_argv);
        puts("164");
    	free_argv(&line_argv);  line_argv = NULL;
        puts("166");
        */
    }
	//-----	
    // clean up
    clear_all_table_elems();
    dictionary_destroy(table);
    clear_neighbor_names(neighbor_names);
    pthread_mutex_destroy(&m);
    return 0;    
}

//--------------------------------------------------------------
void print_usage() {
    fprintf(stderr, "./client bot_number\n");    
}

//--------------------------------------------------------------
int connect_handler_helper(char * host, char * port) {
	struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int s = getaddrinfo(host, port, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(1);
    }

    int sock_fd = socket(hints.ai_family, hints.ai_socktype, 0);
    if (sock_fd == -1) {
        fprintf(stderr, "client: fail to create a socket.\n");
        exit(1);
    }

    int ok = connect(sock_fd, result->ai_addr, result->ai_addrlen);
    if (ok == -1) {
        fprintf(stderr, "client: fail to connect.\n");
        exit(1);
    }

    freeaddrinfo(result);
    return sock_fd;
}

//--------------------------------------------------------------
void * connect_handler(void * _bot_num) {
    sleep(6);
    char * bot_num = (char *) _bot_num;
    char * port1, * port2;
    switch (bot_num[0]) {
        case 'A':
            turn_light_on(bot_num[0], 'B');
            turn_light_on(bot_num[0], 'C');
            port1 = PORT3;
            port2 = PORT2;
            break;
        case 'B':
            turn_light_on(bot_num[0], 'A');
            turn_light_on(bot_num[0], 'D');
            port1 = PORT1;
            port2 = PORT4;
            break;
        case 'C':
            turn_light_on(bot_num[0], 'A');
            turn_light_on(bot_num[0], 'C');
            port1 = PORT4;
            port2 = PORT1;
            break;
        case 'D':
            turn_light_on(bot_num[0], 'B');
            turn_light_on(bot_num[0], 'C');
            port1 = PORT2;
            port2 = PORT3;
            break;
    }
	int sock_fd1 = connect_handler_helper(LOCAL_IP, port1);
	int sock_fd2 = connect_handler_helper(LOCAL_IP, port2);
    send_name(sock_fd1,bot_num);
    send_name(sock_fd2,bot_num);
	return NULL;
}

//--------------------------------------------------------------
void * listen_handler(void * _bot_num) {
    char * bot_num = (char *) _bot_num;
    char * port;
    switch (bot_num[0]) {
        case 'A':
		    port = PORT1;	
            break;
        case 'B':
		    port = PORT2;	
            break;
        case 'C':
		    port = PORT3;	
            break;
        case 'D':
		    port = PORT4;	
            break;
    }
	//-----
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    int s = getaddrinfo(NULL, port, &hints, &result); 
    if (s != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s)); 
            exit(1);
    }
    
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    int optval = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    dictionary_set(table, (void *) (size_t) bot_num[0], (void *) (size_t) sock_fd);

    if (bind(sock_fd, result->ai_addr, result->ai_addrlen) != 0) {
        perror(NULL); 
        exit(1);
    }

    if (listen(sock_fd, NEIGHBOR_NUM) != 0) {
        perror(NULL); 
        exit(1);
    }

    int processed = 0;
    while (processed < NEIGHBOR_NUM) {
        size_t client_fd = accept(sock_fd, NULL, NULL);
        char * client_name = read_name(client_fd);
        dictionary_set(table, (void *) (size_t) client_name[0], (void *) client_fd);
        neighbor_names[processed] = client_name;
        fprintf(stderr, "Bot%s successfully connects to Bot%s\n", bot_name, client_name);
        processed++;
    }
    freeaddrinfo(result);
    // keep accepting
    while (running) {
        size_t client_fd = accept(sock_fd, NULL, NULL);
        char * client_name = read_name(client_fd);
        dictionary_set(table, (void *) (size_t) client_name[0], (void *) client_fd);
        fprintf(stderr, "Bot%s successfully connects to Bot%s\n", bot_name, client_name);
    }
    return NULL;    
}

//--------------------------------------------------------------
void SIGINT_handler() {
    // TODO:
    // send message to its neighbors 
    turn_all_lights_off(bot_name[0]);
    fprintf(stdout, "Bot %c left the network.\n", bot_name[0]);
}

//--------------------------------------------------------------
void SIGPIPE_handler() {
        
}

//--------------------------------------------------------------
char * msg_before_next(char * msg) {
    if (msg == NULL) {
        return NULL;    
    }
    char * rv = calloc(1, strlen(msg) + 1);
    int itr = 0;
    while (msg[itr]) {
        if (msg[itr] == '\n') {
            return rv;    
        }
        rv[itr] = msg[itr];
        itr++;
    }
    print_invalid_msg_format();
    return NULL;
}

//--------------------------------------------------------------
void print_invalid_msg_format() {
    fprintf(stderr, "Incorrect message format.\n");    
}

//--------------------------------------------------------------
ssize_t read_all_from_fd(int fd, char *buffer, size_t count) {
    // Your Code Here
    ssize_t n = 0;
    size_t bytes_done = 0;
	while (bytes_done != count) {
        int errno_saved = errno;
        n = read(fd, buffer + bytes_done, count - bytes_done);
        if (n == 0) { //disconnected
            return 0;    
        }
        else if (n < 0) {
            if  (errno == EINTR) {
                errno = errno_saved;
                continue;    
            }
            else {
                exit(1);
            }
        }
        else { // if (n > 0) 
            bytes_done += n;
        }
	}
    return count;
}

//-------------------------------------------------------------
ssize_t write_all_to_fd(int fd, const char *buffer, size_t count) {
    // Your Code Here
    ssize_t n = 0;
    size_t bytes_done = 0;
	while (bytes_done != count) {
        int errno_saved = errno;
        n = write(fd, buffer + bytes_done, count - bytes_done);
        if (n == 0) { //disconnected
            return 0;    
        }
        else if (n < 0) {
            if  (errno == EINTR) {
                errno = errno_saved;
                continue;    
            }
            else {
                exit(1);
            }
        }
        else { // if (n > 0) 
            bytes_done += n;
        }
	}
    return count;
}

//-------------------------------------------------------------
void send_name(int to_fd, char * name) {
    write_all_to_fd(to_fd, name, 1);
}

//-------------------------------------------------------------
char * read_name(int from_fd) {
    char * name = calloc(1, 2);
    read_all_from_fd(from_fd, name, 1);    
    return name;
}

//-------------------------------------------------------------
void clear_all_table_elems() {
    for (int i=0; i < NEIGHBOR_NUM; i++) {
        if (neighbor_names[i] != NULL &&
            dictionary_contains(table, neighbor_names[i])) {
            dictionary_remove(table, neighbor_names[i]);
        }    
    }        
}

//-------------------------------------------------------------
void clear_neighbor_names(char ** _neighbor_names) {
    for (int i=0; i < NEIGHBOR_NUM; i++) {
        free(_neighbor_names[i]);    
    }    
    free(_neighbor_names);
}

//-------------------------------------------------------------
void send_query(char curr_bot, char query_bot) {
    pthread_mutex_lock(&m);
    message * msg = (message *) calloc(sizeof(message), 1);
    msg->msg_id = msg_id;
    msg->from = curr_bot;
    msg->request = QUERY;
    msg->query_bot = query_bot;
    for (int i = 0; i < TOTAL_BOT_NUM; i++) {
        char this_bot = all_bot_names[i];
        if (this_bot != curr_bot && 
            dictionary_contains(table, (void *) (size_t) this_bot)) {
            msg->to = this_bot;
            int sock_fd = (int) dictionary_get(table, (void *) (size_t) this_bot);
            write_all_to_fd(sock_fd, (char *) msg, sizeof(msg)); 
        }
    }
    pthread_mutex_unlock(&m);
}

//-------------------------------------------------------------
void do_query(char curr_bot) {
    char query_names[TOTAL_BOT_NUM];
    int query_num = 0;
    for (int i = 0; i < TOTAL_BOT_NUM; i++) {
        if (all_bot_names[i] != curr_bot && 
            !dictionary_contains(table, (void *) (size_t) all_bot_names[i])) {
            query_names[query_num++] = all_bot_names[i];
        }
    }
    for (int i = 0; i < query_num; i++) {
        send_query(curr_bot, query_names[i]);    
    }
}

//-------------------------------------------------------------
void read_msg(char curr_bot) {
    
}

//-------------------------------------------------------------
void print_dictionary(dictionary * table) {
    vector * keys = dictionary_keys(table);
    vector * values = dictionary_values(table);
    size_t n = vector_size(keys);
    for (size_t i = 0; i < n; i++) {
        fprintf(stdout, "Key: %c; Value: %d\n", (char) vector_get(keys, i), (int) vector_get(values, i));
    }
    vector_destroy(keys);
    vector_destroy(values);
}

//-------------------------------------------------------------
char * verb_to_string(verb v) {
    char * rv = NULL;
    if (v == QUERY) {
        rv = strdup("QUERY");  
    }
    else if (v == RESPONSE) {
        rv = strdup("RESPONSE");    
    }
    else if (v == MOVE) {
        rv = strdup("MOVE");    
    }
    else if (v == DISCONNECT) {
        rv = strdup("DISCONNECT");    
    }
    assert(0);
    return rv;
}

//-------------------------------------------------------------
verb string_to_verb(char * s) {
    if (!strcmp(s, "QUERY")) {
        return QUERY;    
    }    
    if (!strcmp(s, "RESPONSE")) {
        return RESPONSE;    
    }
    if (!strcmp(s, "MOVE")) {
        return MOVE;    
    }
    if (!strcmp(s, "DISCONNECT")) {
        return DISCONNECT;    
    }
    assert(0);
    return UNKNOWN;
}

//-------------------------------------------------------------
char * shell_getline() {
    size_t capacity = 128;
    char * line = (char *) malloc(capacity);
    size_t size = 0;
    int c;

    if (line == NULL) {
        return NULL;    
    }
    while (1) {
        c = fgetc(stdin);
        if (c == EOF) {
            free(line); line = NULL;
            return NULL;
        }

        if (size == capacity) {
            capacity *= 2;
            char * temp = realloc(line, capacity);
            if (temp == NULL) {
                free(line); line = NULL;
                return NULL;    
            }
            line = temp; temp = NULL;
        }

        line[size++] = c;
        if (c == '\n') {
            line[size - 1] = '\0';
            break;
        }
    }
    return line;
}

//-------------------------------------------------------------
void turn_all_lights_off(char name) {
    int * leds;
    if (bot_name[0] == 'A') {
        leds = A_leds;
    }
    if (bot_name[0] == 'B') {
        leds = B_leds;
    }
    if (bot_name[0] == 'C') {
        leds = C_leds;
    }
    if (bot_name[0] == 'D') {
        leds = D_leds;
    }
    for (size_t i = 0; i < TOTAL_BOT_NUM; i++) {
        digitalWrite(leds[i], 0);    
    }
}

//-------------------------------------------------------------
void * blink_handler(void * _name) {
    int LED;
    char name = (char) (size_t) _name;
    if (name == 'A') {
        LED = 2;    
    }
    else if (name == 'B') {
        LED = 22;
    }
    else if (name == 'C') {
        LED = 6;    
    }
    else if (name == 'D') {
        LED = 16;    
    }
    else {
        assert(0 && "invalid name");    
    }
    
    while (running) {
        digitalWrite(LED, 1);
        delay(100);
        digitalWrite(LED, 0);
        delay(100);
    } 
    return NULL;
}

//-------------------------------------------------------------
int led_lookup(char curr_name, char other_name) {
    int LED;
    if (curr_name == 'A') {
        if (other_name == 'B') {
            LED = 3; 
        }
        else if (other_name == 'C') {
            LED = 4;
        }
        else if (other_name == 'D') {
            LED = 17;
        }
        else {
            assert(0 && "other_name is invalid");     
        }
    } 
    if (curr_name == 'B') {
        if (other_name == 'A') {
            LED = 27;
        }
        else if (other_name == 'C') {
            LED = 10;
        }
        else if (other_name == 'D') {
            LED = 9;
        }
        else {
            assert(0 && "other_name is invalid");     
        }
    } 
    if (curr_name == 'C') {
        if (other_name == 'A') {
            LED = 11;
        }
        else if (other_name == 'B') {
            LED = 5;
        }
        else if (other_name == 'D') {
            LED = 13;
        }
        else {
            assert(0 && "other_name is invalid");     
        }
    } 
    if (curr_name == 'D') {
        if (other_name == 'A') {
            LED = 12;
        }
        else if (other_name == 'B') {
            LED = 21;
        }
        else if (other_name == 'C') {
            LED = 20;
        }
        else {
            assert(0 && "other_name is invalid");     
        }
    } 
    return LED;
}

//-------------------------------------------------------------
void turn_light_off(char curr_name, char other_name) {
    int led = led_lookup(curr_name, other_name); 
    digitalWrite(led, 0);
}

//-------------------------------------------------------------
void turn_light_on(char curr_name, char other_name) {
    int led = led_lookup(curr_name, other_name); 
    digitalWrite(led, 1);
}

//-------------------------------------------------------------
// Split the return value by space from shell_getline();it also removes all extra spaces
char ** shell_split_line(char * line) {
    /*
    sstring * s_line = cstr_to_sstring(line);
    vector * vec = sstring_split(s_line, ' ');
    size_t num_strings = vector_size(vec);
    char ** rv = calloc(num_strings + 1, sizeof(char *));
    size_t itr = 0;
    size_t cpy_itr = 0;
    for (; itr < num_strings; itr++) {
        const char * string = vector_get(vec, itr);
        if (!strcmp(string, " ") ||
            !strcmp(string, "")  ||
            !strcmp(string, "\t")) {
            continue;    
        }
        rv[cpy_itr++] = strdup(string);
    }

    vector_destroy(vec);
    sstring_destroy(s_line);  
    return rv;
    */
    return NULL;
}

//-------------------------------------------------------------
int count_argv(char ** argv) {
    if (!argv) {
        return 0;
    }
    int i=0;
    while (argv[i]) {
        i++;    
    }
    return i;
}

//-------------------------------------------------------------
void free_argv(char *** argv) {
    if (!argv || !(*argv)) {
        return;
    }
    int i = 0;    
    while ((*argv)[i]) {
        free((*argv)[i]); (*argv)[i] = NULL;
        i++;
    }
    free(*argv); *argv = NULL;
}

//-------------------------------------------------------------
void print_argv(char ** argv) {
    int i = 0;
    while (argv[i]) {
        fprintf(stderr, "%d: %s\n", i, argv[i]);
        i++;
    }
}
//-------------------------------------------------------------
//-------------------------------------------------------------
