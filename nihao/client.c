/**
* Networking Lab
* CS 241 - Fall 2018
*/

#include "common.h"
#include "dictionary.h"
#include "format.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>


char **parse_args(int argc, char **argv);
verb check_args(char **args);


typedef struct message{
    int id;
    char to;
    char from;
    int type;
    int subtype;
} message; 



/**
 * te_metadata = Table Entry Metadata
 * <key, value> == <char, te_metadata>
 */
typedef struct te_metadata{
    char direct;
    int fd;
} te_metadata; // 
// Each 



static char my_name; // will be set by passing in argument into command line
// ./client A port port ---> A is the name


static dictionary * table; // global dictionary for our routing table

/**
 * counter for all the messages we've sent
 * this will be used as the ID for each message we send
 * 
 */
static int messages_sent; 



int get_fd_from_table(char key){

    printf("KEY OUT : %c\n", key);
    te_metadata * value = (te_metadata *) dictionary_get(table, (void *) (size_t) key);

    return value->fd;

}


/**
 * TODO: implement the return values
 * returns 1 on success
 * returns -1 on failure
 */
int send_message(char to, int type, int subtype){
    message * package = (message *) malloc(sizeof(message));
    package->id = messages_sent;
    messages_sent += 1;
    package->to = to;
    package->from = my_name;
    package->type = type;
    package->subtype = subtype;


    int fd = get_fd_from_table(to);

    printf("File descriptor recipient : %d\n", fd);
    (void)fd;
    // char tmp_buf[sizeof(message)];

    printf("MESSAGE :\n");
    printf("package->id : %d\n", package->id);
    printf("package->to : %c\n", package->to);
    printf("package->from : %c\n", package->from);
    printf("package->type : %d\n", package->type);
    printf("package->subtype : %d\n", package->subtype);

    return 0;

}




int main(int argc, char **argv) {
    // Good luck!

    table = dictionary_create(shallow_hash_function, shallow_compare, NULL, NULL, NULL, NULL);

    memmove(&my_name, argv[1], 1);
    printf("My name is : %c\n", my_name);
    char my_char = 'B';

    te_metadata * B_meta = (te_metadata *) malloc(sizeof(te_metadata));
    B_meta->direct = -1;
    B_meta->fd = 47;

    printf("KEY IN : %c\n", my_char);
    dictionary_set(table, (void *) (size_t) my_char, (void *) B_meta);


    send_message(my_char, 1, 1);


    // dictionary_destroy(table);



    return 0;
}























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


/********************************************//**
 *  ... extra stuff
 ***********************************************/

// int list[5] = {GET, PUT, DELETE, LIST, V_UNKNOWN};
//     for(int i = 0; i < 5; i ++){
//         printf("%d\n", list[i]);
//     }

// for(int i = 0; i < 5; i++){
//         printf("%s\n", args[i]);
//     }

// char mesg1_buffer[filename_len + 5 + 1 /* the NULL byte */];
//     // int sprintf(char *str, const char *format, ...);
//     int byteswritten = sprintf(mesg1_buffer, "PUT %s\n", remote);
//     (void)byteswritten;
//     printf("%s\n", mesg1_buffer);
//     write(server_fd, mesg1_buffer, filename_len + 5);
//     // printf("WHAT I WROTE TO THE SERVER : ");
//     // fflush(stdout);
//     write(1, mesg1_buffer, filename_len + 5);

//     char my_char = mesg1_buffer[filename_len + 4];
//     printf("my char : %d\n", (int)my_char);

//     // int stat(const char *path, struct stat *buf);
//     struct stat buf;
//     int stat_err = stat(local, &buf);
//     if (stat_err == -1){
//         perror("stat error");
//         exit(1);
//     }
//     size_t file_size = buf.st_size;
//     printf("File size: %zu bytes\n", file_size);
//     char mesg2_buffer[file_size + 1 /* the NULL byte */];
//     (void)mesg2_buffer;



//     // FILE *fopen(const char *pathname, const char *mode);
//     FILE * f = fopen(local, "r");
//     int file_fd = fileno(f);


//      // void *mmap(void *addr, size_t length, int prot, int flags,
//      //                  int fd, off_t offset);
//     char * addr = (char *) mmap(0, file_size, PROT_READ, MAP_SHARED, file_fd, 0);
//     int byteswritten2 = sprintf(mesg2_buffer, "%s", addr);
//     (void)byteswritten2;

//     printf("%s", mesg2_buffer);
//     write(server_fd, mesg2_buffer, sizeof(size_t) + file_size);
//     printf("DONE\n");

//     // printf("THE FILE CONTENTS :\n%s\n", addr);



//     // printf("\n");


