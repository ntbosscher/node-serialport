
#include "./Flush.h"
#include "./VoidBaton.h"

void EIO_Flush(uv_work_t* req) {
    VoidBaton* data = static_cast<VoidBaton*>(req->data);

    DWORD purge_all = PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR;
    if (!PurgeComm(int2handle(data->fd), purge_all)) {
        ErrorCodeToString("Flushing connection (PurgeComm)", GetLastError(), data->errorString);
        return;
    }
}
