/**
* Networking Lab
* CS 241 - Fall 2018
*/

typedef enum { OK, ERROR, R_UNKNOWN } result;

char *verb_to_str(verb command) {
    if (command == LIST) {
        return "LIST";
    }
    if (command == GET) {
        return "GET";
    }
    if (command == PUT) {
        return "PUT";
    }
    if (command == DELETE) {
        return "DELETE";
    }
    return "UNKNOWN";
}

verb str_to_verb(char *command) {
    if (strcmp(command, "LIST") == 0) {
        return LIST;
    }
    if (strcmp(command, "GET") == 0) {
        return GET;
    }
    if (strcmp(command, "PUT") == 0) {
        return PUT;
    }
    if (strcmp(command, "DELETE") == 0) {
        return DELETE;
    }
    return V_UNKNOWN;
}

char *result_to_str(result status) {
    if (status == OK) {
        return "OK";
    }
    if (status == ERROR) {
        return "ERROR";
    }
    return "UNKNOWN";
}

result str_to_result(char *status) {
    if (strcmp(status, "OK") == 0) {
        return OK;
    }
    if (strcmp(status, "ERROR") == 0) {
        return ERROR;
    }
    return R_UNKNOWN;
}
