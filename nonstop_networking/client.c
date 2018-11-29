/**
* Networking Lab
* CS 241 - Fall 2018
*/
//--------------------------------------------------------------
#include "common.h"
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
//--------------------------------------------------------------
static int sock_fd;
//--------------------------------------------------------------
char **parse_args(int argc, char **argv);
verb check_args(char **args);
int connect_to_server(char * host, char * port);

char * create_client_msg(verb request, char ** args, size_t * out_len);
size_t get_msg_size();
char * get_response();
char * get_error_msg();

void SIGPIPE_handler(int signum) {
    (void) signum;
    print_connection_closed();
    exit(1);
}
void write_to_server(verb request, char ** args);
void read_from_server(verb request, char ** args);

void rw_big_file(int from, int to, ssize_t file_size);
//-------------------------------------------------------------
int main(int argc, char **argv) {
    // process and validate arguments
    char ** args = parse_args(argc, argv);
    verb request = check_args(args);
    // connect
    sock_fd = connect_to_server(args[0], args[1]);
    // write
    signal(SIGPIPE, SIGPIPE_handler);
    write_to_server(request, args);
    shutdown(sock_fd, SHUT_WR);
    // read 
    read_from_server(request, args);
    // clean up
    close(sock_fd);
    free(args);
}

//--------------------------------------------------------------
// DONE
/**
 * Given commandline argc and argv, parses argv.
 *
 * argc argc from main()
 * argv argv from main()
 *
 * Returns char* array in form of {host, port, method, remote, local, NULL}
 * where `method` is ALL CAPS
 */
char **parse_args(int argc, char **argv) {
    if (argc < 3) {
        return NULL;
    }

    char *host = strtok(argv[1], ":");
    char *port = strtok(NULL, ":");
    if (port == NULL) {
        return NULL;
    }

    char **args = calloc(1, 6 * sizeof(char *));
    args[0] = host;
    args[1] = port;
    args[2] = argv[2];
    char *temp = args[2];
    while (*temp) {
        *temp = toupper((unsigned char)*temp);
        temp++;
    }
    if (argc > 3) {
        args[3] = argv[3];
    }
    if (argc > 4) {
        args[4] = argv[4];
    }

    return args;
}

//--------------------------------------------------------------
// DONE
/**
 * Validates args to program.  If `args` are not valid, help information for the
 * program is printed.
 *
 * args     arguments to parse
 *
 * Returns a verb which corresponds to the request method
 */
verb check_args(char **args) {
    if (args == NULL) {
        print_client_usage();
        exit(1);
    }

    char *command = args[2];

    if (strcmp(command, "LIST") == 0) {
        return LIST;
    }

    if (strcmp(command, "GET") == 0) {
        if (args[3] != NULL && args[4] != NULL) {
            return GET;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "DELETE") == 0) {
        if (args[3] != NULL) {
            return DELETE;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "PUT") == 0) {
        if (args[3] == NULL || args[4] == NULL) {
            print_client_help();
            exit(1);
        }
        return PUT;
    }

    // Not a valid Method
    print_client_help();
    exit(1);
}

//--------------------------------------------------------------
// DONE
int connect_to_server(char * host, char * port) {
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
// DONE
char * get_response() {
    // 6 == strlen("ERROR") + 1
    char * rv = calloc(1, 6);
    char next[2];
    next[1] = '\0';
    if (read_all_from_fd(sock_fd, rv, 2) != 2) {
        print_invalid_response();
        exit(1);
    }
    if (!strcmp(rv, "OK")) {
        if (read_all_from_fd(sock_fd, next, 1) != 1 ||
            next[0] != '\n') {
            print_invalid_response();
            exit(1);
        }
        return rv;
    }
    if (read_all_from_fd(sock_fd, rv + 2, 3) != 3 ||
        strcmp(rv, "ERROR") ||
        read_all_from_fd(sock_fd, next, 1) != 1 ||
        next[0] != '\n') {
        print_invalid_response();
        exit(1);
    }
    return rv;
}

//--------------------------------------------------------------
// DONE
char * get_error_msg() {
    char * error_msg = calloc(1, 1024);
    read_all_from_fd(sock_fd, error_msg, 1024);
    for (size_t i = 0; i < 1024; i++) {
        if (error_msg[i] == '\n') {
            error_msg[i] = '\0';
            return error_msg;
        }
    }
    print_invalid_response();
    exit(1);
}

//--------------------------------------------------------------
// DONE
size_t get_msg_size() {
    size_t size;
    if ( 0 == read_all_from_fd(sock_fd, (char *) &size, sizeof(size_t)) ) {
        print_invalid_response();
    }
    return size;
}

//--------------------------------------------------------------
// IMPORTANT: For PUT, return value includes everything except the raw bytes of the file
// args: {host, port, method, remote, local, NULL}
char * create_client_msg(verb request, char ** args, size_t * out_len) {
    char * out_msg = NULL;

    if (request == GET) {
        size_t file_len = strlen(args[3]);
        *out_len = 4 + file_len + 1;
        out_msg = calloc(1, *out_len);
        memcpy(out_msg, "GET ", 4);
        memcpy(out_msg + 4, args[3], file_len);
        memcpy(out_msg + 4 + file_len, "\n", 1);
    }
    else if (request == DELETE) {
        size_t file_len = strlen(args[3]);
        *out_len = 7 + file_len + 1;
        out_msg = calloc(1, *out_len);
        memcpy(out_msg, "DELETE ", 7);
        memcpy(out_msg + 7, args[3], file_len);
        memcpy(out_msg + 7 + file_len, "\n", 1);
    }
    else if (request == LIST) {
        *out_len = 5;
        out_msg = calloc(1, *out_len);
        memcpy(out_msg, "LIST\n", *out_len);
    }
    else if (request == PUT) {
        size_t name_len = strlen(args[3]);
        *out_len = 4 + name_len + 1 + sizeof(size_t);
        out_msg = calloc(1, *out_len);
        memcpy(out_msg, "PUT ", 4);
        memcpy(out_msg + 4, args[3], name_len);
        memcpy(out_msg + 4 + name_len, "\n", 1);

        struct stat s;
        if (stat(args[4], &s) == -1) {
            fprintf(stderr, "client: the file does not exit.\n");
            exit(1);
        }
        size_t file_size = s.st_size;
        memcpy(out_msg + 4 + name_len + 1, (char *) &file_size, sizeof(size_t));
    }
    return out_msg;
}

//--------------------------------------------------------------
void write_to_server(verb request, char ** args) {
    // creat the message to send
    size_t out_len;
    char * out_msg = create_client_msg(request, args, &out_len);

    ssize_t bytes_written = write_all_to_fd(sock_fd, out_msg, out_len);
    if (bytes_written == 0) {
        print_connection_closed();
        exit(1);
    }
    free(out_msg); out_msg = NULL;
    if (request == PUT) {
        int local_fd = open(args[4], O_RDONLY);
        struct stat s;
        if (fstat(local_fd, &s) == -1) {
            fprintf(stderr, "fstat failed\n");
            exit(1);
        }
        rw_big_file(local_fd, sock_fd, s.st_size);
        close(local_fd);
    }
}

//--------------------------------------------------------------
// DONE
void read_from_server(verb request, char ** args) {
    char * response = get_response();
    if (!strcmp(response, "ERROR")) {
        char * error_msg = get_error_msg();
        print_error_message(error_msg);
        free(error_msg);
    }
    else if (!strcmp(response, "OK") && request == LIST) {
        size_t msg_size = get_msg_size();
        if (msg_size == 0) {
            free(response); response = NULL;
            return;
        }
        if (msg_size > 1024) {
            print_invalid_response();
            exit(1);
        }
        char * in_msg = malloc(msg_size);
        char extra[0];
        if (read_all_from_fd(sock_fd, in_msg, msg_size) == 0) {
            print_too_little_data();
        }
        else if (read_all_from_fd(sock_fd, extra, 1) != 0) {
            print_received_too_much_data();
        }
        else {
            write_all_to_fd(1, in_msg, msg_size);
        }
        free(in_msg); in_msg = NULL;
    }
    else if (!strcmp(response, "OK") && request == GET) {
        size_t msg_size = get_msg_size();
        if (msg_size == 0) {
            print_invalid_response();
            exit(1);
        }
        mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;
        int local_fd = open(args[4], O_CREAT | O_TRUNC | O_RDWR, mode);
        rw_big_file(sock_fd, local_fd, msg_size);
        close(local_fd);
    }
    else {
        print_success();    
    }
    free(response); response = NULL;
}

//--------------------------------------------------------------
// DONE
void rw_big_file(int from, int to, ssize_t file_size) {
    struct stat s;
    if (fstat(to, &s) == -1) {
        fprintf(stderr, "fstat failed\n");
        exit(1);
    }

    char * buf = malloc(s.st_blksize);
    ssize_t written = 0;
    while (written < file_size) {
        size_t read_size;
        if (file_size - written < s.st_blksize) {
            read_size = file_size % s.st_blksize;
        }
        else {
            read_size = s.st_blksize;    
        }
        if (read_all_from_fd(from, buf, read_size) == 0) {
            print_too_little_data();
            free(buf); 
            return;
        } 
        write_all_to_fd(to, buf, read_size);
        written += read_size;
    }

    if (from == sock_fd) {
        char extra[0];
        if (read_all_from_fd(sock_fd, extra, 1) != 0) {
            print_received_too_much_data();
        }
    }
    free(buf); buf = NULL;
}
