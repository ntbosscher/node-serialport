
#include "ReadBaton.h"
#include <nan.h>
#include "win.h"

v8::Local<v8::Value> ReadBaton::getReturnValue()
{
    return Nan::New<v8::Integer>(static_cast<int>(bytesRead));
}

int readHandle(ReadBaton *baton, bool blocking)
{
    OVERLAPPED *ov = new OVERLAPPED;

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

    if (!ReadFile(int2handle(baton->fd), offsetPtr, baton->bytesToRead, &bytesTransferred, ov))
    {
        int errorCode = GetLastError();

        if (errorCode != ERROR_IO_PENDING)
        {
            ErrorCodeToString("Reading from COM port (ReadFile)", errorCode, baton->errorString);
            baton->complete = true;
            CloseHandle(ov->hEvent);
            return 0;
        }

        if (!GetOverlappedResult(int2handle(baton->fd), ov, &bytesTransferred, TRUE))
        {
            int lastError = GetLastError();
            ErrorCodeToString("Reading from COM port (GetOverlappedResult)", lastError, baton->errorString);
            baton->complete = true;
            CloseHandle(ov->hEvent);
            return 0;
        }
    }

    CloseHandle(ov->hEvent);

    baton->bytesToRead -= bytesTransferred;
    baton->bytesRead += bytesTransferred;
    baton->offset += bytesTransferred;
    baton->complete = baton->bytesToRead == 0;
    return bytesTransferred;
}

void ReadBaton::run()
{

    int start = currentMs();
    int deadline = start + this->timeout;

    do
    {
        readHandle(this, true);

        if (bytesToRead == 0)
        {
            complete = true;
        }

    } while (!complete && currentMs() < deadline);
}

NAN_METHOD(Read)
{
    // file descriptor
    if (!info[0]->IsInt32())
    {
        Nan::ThrowTypeError("First argument must be a fd");
        return;
    }
    int fd = Nan::To<int>(info[0]).FromJust();

    // buffer
    if (!info[1]->IsObject() || !node::Buffer::HasInstance(info[1]))
    {
        Nan::ThrowTypeError("Second argument must be a buffer");
        return;
    }
    v8::Local<v8::Object> buffer = Nan::To<v8::Object>(info[1]).ToLocalChecked();
    size_t bufferLength = node::Buffer::Length(buffer);

    // offset
    if (!info[2]->IsInt32())
    {
        Nan::ThrowTypeError("Third argument must be an int");
        return;
    }
    int offset = Nan::To<v8::Int32>(info[2]).ToLocalChecked()->Value();

    // bytes to read
    if (!info[3]->IsInt32())
    {
        Nan::ThrowTypeError("Fourth argument must be an int");
        return;
    }

    size_t bytesToRead = Nan::To<v8::Int32>(info[3]).ToLocalChecked()->Value();

    if ((bytesToRead + offset) > bufferLength)
    {
        Nan::ThrowTypeError("'bytesToRead' + 'offset' cannot be larger than the buffer's length");
        return;
    }

    // timeout
    if (!info[4]->IsInt32())
    {
        Nan::ThrowTypeError("Fifth argument must be an int");
        return;
    }

    int timeout = Nan::To<v8::Int32>(info[4]).ToLocalChecked()->Value();

    // callback
    if (!info[5]->IsFunction())
    {
        Nan::ThrowTypeError("Sixth argument must be a function");
        return;
    }

    auto cb = Nan::To<v8::Function>(info[5]).ToLocalChecked();
    ReadBaton *baton = new ReadBaton("ReadBaton", cb);

    baton->fd = fd;
    baton->offset = offset;
    baton->bytesToRead = bytesToRead;
    baton->bufferLength = bufferLength;
    baton->bufferData = node::Buffer::Data(buffer);
    baton->timeout = timeout;
    baton->complete = false;

    baton->start();
}