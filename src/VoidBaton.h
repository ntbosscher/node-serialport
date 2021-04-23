
#include <nan.h>
#include <v8.h>
#include "./Util.h"

#ifndef FASTER_SERIALPORT_VOIDBATON_H
#define FASTER_SERIALPORT_VOIDBATON_H

struct VoidBaton : public Nan::AsyncResource {
    VoidBaton() : AsyncResource("node-serialport:VoidBaton"), errorString() {}
    int fd = 0;
    Nan::Callback callback;
    char errorString[ERROR_STRING_SIZE];
};

#endif //FASTER_SERIALPORT_VOIDBATON_H
