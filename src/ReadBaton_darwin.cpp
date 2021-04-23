
#include "./ReadBaton.h"
#include <unistd.h>
#include <fcntl.h>

int readFromSerial(int fd, char* buffer, int length, bool blocking, char* error) {
    if(!blocking) {
        strcpy(error, "blocking=false not implimented");
        return -1;
    }

    int flags = fcntl(fd, F_GETFL, 0);
    if(flags & O_NONBLOCK) {
        if(fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) == -1) {
            sprintf(error, "Failed set file flags (errno=%d)", errno);
            return -1;
        }
    }

    int result = read(fd, buffer, length);
    if(result >= 0) return result;

    if(result == -1) {
        sprintf(error, "Failed to read (errno=%d)", errno);
        return result;
    }
}