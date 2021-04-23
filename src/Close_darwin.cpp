
#include "./Close.h"
#include <unistd.h>
#include "./VoidBaton.h"

void EIO_Close(uv_work_t* req) {
    VoidBaton* data = static_cast<VoidBaton*>(req->data);

    if (-1 == close(data->fd)) {
        snprintf(data->errorString, sizeof(data->errorString),
                 "Error: %s, unable to close fd %d", strerror(errno), data->fd);
    }
}