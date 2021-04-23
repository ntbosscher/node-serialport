
#include <nan.h>

#ifndef FASTER_SERIALPORT_DRAIN_H
#define FASTER_SERIALPORT_DRAIN_H

NAN_METHOD(Drain);
void EIO_Drain(uv_work_t* req);
void EIO_AfterDrain(uv_work_t* req);

#endif //FASTER_SERIALPORT_DRAIN_H
