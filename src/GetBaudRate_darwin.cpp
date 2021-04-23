
#include "./GetBaudRate.h"

void EIO_GetBaudRate(uv_work_t* req) {
    GetBaudRateBaton* data = static_cast<GetBaudRateBaton*>(req->data);
    int outbaud = -1;

#if defined(__linux__) && defined(ASYNC_SPD_CUST)
    if (-1 == linuxGetSystemBaudRate(data->fd, &outbaud)) {
    snprintf(data->errorString, sizeof(data->errorString), "Error: %s, cannot get baud rate", strerror(errno));
    return;
  }
#else
    snprintf(data->errorString, sizeof(data->errorString), "Error: System baud rate check not implemented on this platform");
    return;
#endif

    data->baudRate = outbaud;
}