
#include "./ReadBaton.h"
#include "./Darwin.h"
#include <unistd.h>
#include <fcntl.h>

int readFromSerial(int fd, char* buffer, int length, bool blocking, char* error) {

    if(setFileBlockingFlags(fd, blocking, error) == -1) {
        return -1;
    }

    int result = read(fd, buffer, length);
    if(result >= 0) return result;

    if(result == -1) {
        if(!blocking) {
            // normal errors that signal data isn't ready yet
            switch(errno) {
                case EAGAIN:
                    errno = 0;
                    return 0;
            }
        }

        sprintf(error, "Failed to read (errno=%d)", errno);
    }

    return result;
}