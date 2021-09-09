
#include "ReadBaton.h"
#include <nan.h>
#include "Win.h"
#include "./Util.h"
#include "./V8ArgDecoder.h"


int readFromSerial(int fd, char* buffer, int length, bool blocking, char* error)
{
    
    auto ov = unique_ptr<OVERLAPPED>(new OVERLAPPED());

    // Set the timeout to MAXDWORD in order to disable timeouts, so the read operation will
    // return immediately no matter how much data is available.
    COMMTIMEOUTS commTimeouts = {};

    if (blocking)
    {
        commTimeouts.ReadTotalTimeoutConstant = TIMEOUT_PRECISION;
    }
    else
    {
        commTimeouts.ReadIntervalTimeout = MAXDWORD;
    }

    if (!SetCommTimeouts(int2handle(fd), &commTimeouts))
    {
        int lastError = GetLastError();
        ErrorCodeToString("Setting COM timeout (SetCommTimeouts)", lastError, error);
        return -1;
    }

    // ReadFile, unlike ReadFileEx, needs an event in the overlapped structure.
    memset(ov.get(), 0, sizeof(OVERLAPPED));
    ov->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    DWORD bytesTransferred = 0;

    if (!ReadFile(int2handle(fd), buffer, length, &bytesTransferred, ov.get()))
    {
        int errorCode = GetLastError();
        if (errorCode != ERROR_IO_PENDING)
        {
            ErrorCodeToString("Reading from COM port (ReadFile)", errorCode, error);
            CloseHandle(ov->hEvent);
            return -1;
        }

        if (!GetOverlappedResult(int2handle(fd), ov.get(), &bytesTransferred, TRUE))
        {
            int lastError = GetLastError();
            ErrorCodeToString("Reading from COM port (GetOverlappedResult)", lastError, error);
            CloseHandle(ov->hEvent);
            return -1;
        }
    }

    CloseHandle(ov->hEvent);
    return bytesTransferred;
}