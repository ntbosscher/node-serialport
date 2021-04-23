
#include <nan.h>

#ifndef FASTER_SERIALPORT_FLUSH_H
#define FASTER_SERIALPORT_FLUSH_H

NAN_METHOD(Flush);
void EIO_Flush(uv_work_t* req);
void EIO_AfterFlush(uv_work_t* req);

#endif //FASTER_SERIALPORT_FLUSH_H
