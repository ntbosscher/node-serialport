
#include "./Get.h"
#include <sys/ioctl.h>

void EIO_Get(uv_work_t* req) {
    GetBaton* data = static_cast<GetBaton*>(req->data);

    int bits;
    if (-1 == ioctl(data->fd, TIOCMGET, &bits)) {
        snprintf(data->errorString, sizeof(data->errorString), "Error: %s, cannot get", strerror(errno));
        return;
    }

    data->lowLatency = false;
#if defined(__linux__) && defined(ASYNC_LOW_LATENCY)
    int latency_bits;
  if (-1 == ioctl(data->fd, TIOCGSERIAL, &latency_bits)) {
    snprintf(data->errorString, sizeof(data->errorString), "Error: %s, cannot get", strerror(errno));
    return;
  }
  data->lowLatency = latency_bits & ASYNC_LOW_LATENCY;
#endif

    data->cts = bits & TIOCM_CTS;
    data->dsr = bits & TIOCM_DSR;
    data->dcd = bits & TIOCM_CD;
}