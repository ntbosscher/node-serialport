
#include "WriteBaton.h"
#include <nan.h>
#include "win.h"
#include <v8.h>
#include "./V8ArgDecoder.h"

v8::Local<v8::Value> WriteBaton::getReturnValue()
{
    return Nan::Null();
}

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

void WriteBaton::run()
{

    int start = currentMs();
    int deadline = start + this->timeout;

    do
    {
        if(verbose) {
            auto out = defaultLogger();
            out << currentMs() << " " << debugName << " to-write=" << (bufferLength - offset) << " blocking=" << true << " \n";
            out << currentMs() << " " << debugName << " buffer-contents: " << bufferToHex(bufferData, (bufferLength - offset)) << "\n";
            out.close();
        }

        char *buffer = bufferData + offset;
        int len = bufferLength - offset;

        auto bytesTransferred = writeToSerial(fd, buffer, len, true, errorString);
        if(bytesTransferred < 0) {
            complete = true;
            return;
        }

        if(verbose) {
            auto out = defaultLogger();
            out << currentMs() << " " << debugName << " wrote=" << bytesTransferred << " \n";
            out.close();
        }

        offset += bytesTransferred;
        complete = offset == bufferLength;

    } while (!complete && currentMs() > deadline);

    if(!complete) {
        sprintf((char*)&errorString, "Timeout writing to port: %d of %d bytes written", (this->bufferLength-this->offset), this->bufferLength);
    }
}

NAN_METHOD(Write)
{
    V8ArgDecoder args(&info);

    auto fd = args.takeInt32();
    auto buffer = args.takeBuffer();
    auto timeout = args.takeInt32();
    auto cb = args.takeFunction();

    if(args.hasError()) return;

    WriteBaton *baton = new WriteBaton("write-baton", cb);
    
    baton->fd = fd;
    baton->timeout = timeout;
    baton->bufferData = buffer.buffer;
    baton->bufferLength = buffer.length;
    baton->offset = 0;
    baton->complete = false;

    baton->start();
}