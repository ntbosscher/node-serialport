
#include "./Get.h"
#include "./Win.h"

void EIO_Get(uv_work_t* req) {
    GetBaton* data = static_cast<GetBaton*>(req->data);

    DWORD bits = 0;
    if (!GetCommModemStatus(int2handle(data->fd), &bits)) {
        ErrorCodeToString("Getting control settings on COM port (GetCommModemStatus)", GetLastError(), data->errorString);
        return;
    }

    data->cts = bits & MS_CTS_ON;
    data->dsr = bits & MS_DSR_ON;
    data->dcd = bits & MS_RLSD_ON;
}