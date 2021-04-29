
#include <v8.h>
#include <nan.h>
#include "./Util.h"

#ifndef FASTER_SERIALPORT_GET_H
#define FASTER_SERIALPORT_GET_H

struct GetBaton : public Nan::AsyncResource {
    GetBaton() : AsyncResource("node-serialport:GetBaton"), errorString() {}
    int fd = 0;
    Nan::Callback callback;
    char errorString[ERROR_STRING_SIZE];
    bool cts = false;
    bool dsr = false;
    bool dcd = false;
    bool lowLatency = false;
};

NAN_METHOD(Get);
void EIO_Get(uv_work_t* req);
void EIO_AfterGet(uv_work_t* req);

#endif //FASTER_SERIALPORT_GET_H
