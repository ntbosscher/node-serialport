
#include "./WriteBaton.h"
#include "./Darwin.h"
#include <unistd.h>
#include <fcntl.h>

int writeToSerial(int fd, char* buffer, int length, bool blocking, char* error) {
    if(setFileBlockingFlags(fd, blocking, error) == -1) {
        return -1;
    }

    int result = write(fd, buffer, length);
    if(result >= 0) return result;

    if(result == -1) {
        sprintf(error, "Failed to read (errno=%d)", errno);
    }

    return result;
}
