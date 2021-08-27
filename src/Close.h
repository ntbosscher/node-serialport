
#include <nan.h>
#include <v8.h>

#ifndef FASTER_SERIALPORT_CLOSE_H
#define FASTER_SERIALPORT_CLOSE_H

NAN_METHOD(Close);
void EIO_Close(uv_work_t* req);
void EIO_AfterClose(uv_work_t* req);
bool ClosePort(HANDLE fd);

#endif //FASTER_SERIALPORT_CLOSE_H
