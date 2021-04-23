
#include "WriteBaton.h"
#include <nan.h>
#include "Win.h"
#include <v8.h>
#include "./ReadBaton.h"
#include "./V8ArgDecoder.h"
#include <sstream>
#include <iomanip>

int writeToSerial(int fd, char* buffer, int length, bool blocking, char* error) {
    OVERLAPPED *ov = new OVERLAPPED;

    // Set the timeout to MAXDWORD in order to disable timeouts, so the read operation will
    // return immediately no matter how much data is available.
    COMMTIMEOUTS commTimeouts = {};

    if (blocking)
    {
        commTimeouts.ReadIntervalTimeout = TIMEOUT_PRECISION;
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
    memset(ov, 0, sizeof(OVERLAPPED));
    ov->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    DWORD bytesTransferred = 0;

    if (!WriteFile(int2handle(fd), buffer, length, NULL, ov))
    {
        int errorCode = GetLastError();
        if (errorCode != ERROR_IO_PENDING)
        {
            ErrorCodeToString("Writing to COM port (WriteFileEx)", errorCode, error);
            return -1;
        }

        if (!GetOverlappedResult(int2handle(fd), ov, &bytesTransferred, true))
        {
            int errorCode = GetLastError();
            ErrorCodeToString("Writing to COM port (WriteFileEx)", errorCode, error);
            return -1;
        }
    }

    CloseHandle(ov->hEvent);
    return bytesTransferred;
}