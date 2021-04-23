
#include <nan.h>
#include <v8.h>
#include "./Util.h"

#ifndef FASTER_SERIALPORT_GETBAUDRATE_H
#define FASTER_SERIALPORT_GETBAUDRATE_H

struct GetBaudRateBaton : public Nan::AsyncResource {
    GetBaudRateBaton() :
            AsyncResource("node-serialport:GetBaudRateBaton"), errorString() {}
    int fd = 0;
    Nan::Callback callback;
    char errorString[ERROR_STRING_SIZE];
    int baudRate = 0;
};

NAN_METHOD(GetBaudRate);
void EIO_GetBaudRate(uv_work_t* req);
void EIO_AfterGetBaudRate(uv_work_t* req);

#endif //FASTER_SERIALPORT_GETBAUDRATE_H
