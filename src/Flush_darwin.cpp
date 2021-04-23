
#include "./Flush.h"
#include "./VoidBaton.h"

void EIO_Flush(uv_work_t* req) {
    VoidBaton* data = static_cast<VoidBaton*>(req->data);

    if (-1 == tcflush(data->fd, TCIOFLUSH)) {
        snprintf(data->errorString, sizeof(data->errorString), "Error: %s, cannot flush", strerror(errno));
        return;
    }
}