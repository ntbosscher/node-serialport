
#include "WriteBaton.h"
#include <nan.h>
#include "win.h"


v8::Local<v8::Value> WriteBaton::getReturnValue()
{
    buffer.Reset();
    return Nan::Null();
}

int writeHandle(WriteBaton *baton, bool blocking)
{

    if(baton->verbose) {
        auto out = defaultLogger();
        out << currentMs() << " " << baton->debugName << " to-write=" << (baton->bufferLength - baton->offset) << " blocking=" << blocking << " \n";
        out << currentMs() << " " << baton->debugName << " buffer-contents: " << bufferToHex(baton->bufferData, (baton->bufferLength - baton->offset)) << "\n";
        out.close();
    }

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

    if (!SetCommTimeouts(int2handle(baton->fd), &commTimeouts))
    {
        int lastError = GetLastError();
        ErrorCodeToString("Setting COM timeout (SetCommTimeouts)", lastError, baton->errorString);
        baton->complete = true;
        return 0;
    }

    // Store additional data after whatever data has already been read.
    char *offsetPtr = baton->bufferData + baton->offset;

    // ReadFile, unlike ReadFileEx, needs an event in the overlapped structure.
    memset(ov, 0, sizeof(OVERLAPPED));
    ov->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    DWORD bytesTransferred = 0;

    if (!WriteFile(int2handle(baton->fd), offsetPtr, baton->bufferLength - baton->offset, NULL, ov))
    {
        int errorCode = GetLastError();
        if (errorCode != ERROR_IO_PENDING)
        {
            ErrorCodeToString("Writing to COM port (WriteFileEx)", errorCode, baton->errorString);
            baton->complete = true;
            return 0;
        }

        if (!GetOverlappedResult(int2handle(baton->fd), ov, &bytesTransferred, true))
        {
            int errorCode = GetLastError();
            ErrorCodeToString("Writing to COM port (WriteFileEx)", errorCode, baton->errorString);
            baton->complete = true;
            return 0;
        }
    }

    CloseHandle(ov->hEvent);

    if(baton->verbose) {
        auto out = defaultLogger();
        out << currentMs() << " " << baton->debugName << " wrote=" << bytesTransferred << " \n";
        out.close();
    }

    baton->offset += bytesTransferred;
    baton->complete = baton->offset == baton->bufferLength;
    return bytesTransferred;
}

void WriteBaton::run()
{

    int start = currentMs();
    int deadline = start + this->timeout;

    do
    {

        writeHandle(this, true);

        if (this->bytesWritten == this->bufferLength)
            this->complete = true;

    } while (!this->complete && currentMs() > deadline);

    if(!this->complete) {
        sprintf((char*)&errorString, "Timeout writing to port: %d of %d bytes written", this->bytesWritten, this->bufferLength);
    }
}

NAN_METHOD(Write)
{
    // file descriptor
    if (!info[0]->IsInt32()) {
        Nan::ThrowTypeError("First argument must be an int");
        return;
    }

    int fd = Nan::To<int>(info[0]).FromJust();
    
    // buffer
    if (!info[1]->IsObject() || !node::Buffer::HasInstance(info[1])) {
        Nan::ThrowTypeError("Second argument must be a buffer");
        return;
    }

    v8::Local<v8::Object> buffer = Nan::To<v8::Object>(info[1]).ToLocalChecked();
    char* bufferData = node::Buffer::Data(buffer);
    size_t bufferLength = node::Buffer::Length(buffer);

    // timeout
    if (!info[2]->IsInt32()) {
        Nan::ThrowTypeError("Second argument must be an int");
        return;
    }

    int timeout = Nan::To<v8::Int32>(info[2]).ToLocalChecked()->Value();

    // callback
    if (!info[3]->IsFunction()) {
        Nan::ThrowTypeError("Third argument must be a function");
        return;
    }

    auto cb = Nan::To<v8::Function>(info[3]).ToLocalChecked();
    WriteBaton *baton = new WriteBaton("write-baton", cb);
    
    baton->fd = fd;
    baton->timeout = timeout;
    baton->buffer.Reset(buffer);
    baton->bufferData = bufferData;
    baton->bufferLength = bufferLength;
    baton->offset = 0;
    baton->complete = false;

    baton->start();
}