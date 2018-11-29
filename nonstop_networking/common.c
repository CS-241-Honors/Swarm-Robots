/**
* Networking Lab
* CS 241 - Fall 2018
*/

#include "common.h"
//-------------------------------------------------------------
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
