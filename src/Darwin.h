
#include "./Util.h"

#ifndef DARWIN_H
#define DARWIN_H

struct ConnectionOptions {
    ConnectionOptions() : errorString() {}
    char errorString[ERROR_STRING_SIZE];
    int fd = 0;
    int baudRate = 0;
};

int setBaudRate(ConnectionOptions *data);

#endif