
#include "./Drain.h"
#include "./VoidBaton.h"

void EIO_Drain(uv_work_t* req) {
    VoidBaton* data = static_cast<VoidBaton*>(req->data);

    if (-1 == tcdrain(data->fd)) {
        snprintf(data->errorString, sizeof(data->errorString), "Error: %s, cannot drain", strerror(errno));
        return;
    }
}