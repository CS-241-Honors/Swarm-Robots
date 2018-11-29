#include <string.h>
#include <stdio.h>
#include "common.h"

//replace '\n' by '\0' and return the position of '\n'
int remove_next_line(char * input) {
    int itr = 0;
    if (input[itr] == '\n') {
        input[itr] = '\0';
        return itr;
    }
    itr++;
    return 0; 
}

int set_name(char * name) {
    printf("Enter a name with length greater than 0: ");
    if ( fgets(name, MAX_NAME_LENGTH, stdin) ) {
        if (name[0] == '\n') {
            printf("The length of the name must be greater than 0.\n");
        }
        else {
            remove_next_line(name);
            return -1;
        }
    }
    else {
        printf("ERROR: fgets failed");
        return -1;
    }
    return 0;
}
