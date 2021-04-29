
#include "./Drain.h"
#include "./VoidBaton.h"
#include "./Win.h"

void EIO_Drain(uv_work_t* req) {
    VoidBaton* data = static_cast<VoidBaton*>(req->data);

    if (!FlushFileBuffers(int2handle(data->fd))) {
        ErrorCodeToString("Draining connection (FlushFileBuffers)", GetLastError(), data->errorString);
        return;
    }
}