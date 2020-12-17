
#include "ReadBaton.h"
#include <nan.h>
#include "win.h"
#include "./util.h"
#include "./V8ArgDecoder.h"

v8::Local<v8::Value> ReadBaton::getReturnValue()
{
    return Nan::New<v8::Integer>(static_cast<int>(bytesRead));
}

int readFromSerial(int fd, char* buffer, int length, bool blocking, char* error)
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

    if (!ReadFile(int2handle(fd), buffer, length, &bytesTransferred, ov))
    {
        int errorCode = GetLastError();
        if (errorCode != ERROR_IO_PENDING)
        {
            ErrorCodeToString("Reading from COM port (ReadFile)", errorCode, error);
            CloseHandle(ov->hEvent);
            return -1;
        }

        if (!GetOverlappedResult(int2handle(fd), ov, &bytesTransferred, TRUE))
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

void ReadBaton::run()
{

    int start = currentMs();
    int deadline = start + this->timeout;

    do
    {
        if(verbose) {
            auto out = defaultLogger();
            out << currentMs() << " " << debugName << " expecting=" << bytesToRead << " have=" << bytesRead << " blocking=" << true << " \n";
            out.close();
        }

        char *buffer = bufferData + offset;

        auto bytesTransferred = readFromSerial(fd, buffer, bytesToRead, true, errorString);
        if(bytesTransferred < 0) {
            complete = true;
            return; // error
        }

        bytesToRead -= bytesTransferred;
        bytesRead += bytesTransferred;
        offset += bytesTransferred;
        complete = bytesToRead == 0;

        if(verbose) {
            auto out = defaultLogger();
            out << currentMs() << " " << debugName << " got " << bytesTransferred << "bytes\n";
            out << currentMs() << " " << debugName << " buffer-contents: " << bufferToHex(bufferData, bytesRead);

            out << "\n";
            out.close();
        }

    } while (!complete && currentMs() < deadline);
}

NAN_METHOD(Read)
{
    V8ArgDecoder args(&info);

    auto fd = args.takeInt32();
    auto buffer = args.takeBuffer();
    auto offset = args.takeInt32();
    auto bytesToRead = args.takeInt32();
    auto timeout = args.takeInt32();
    auto cb = args.takeFunction();

    if(args.hasError()) return;

    if ((bytesToRead + offset) > buffer.length)
    {
        Nan::ThrowTypeError("'bytesToRead' + 'offset' cannot be larger than the buffer's length");
        return;
    }

    ReadBaton *baton = new ReadBaton("read-baton", cb);

    baton->fd = fd;
    baton->offset = offset;
    baton->bytesToRead = bytesToRead;
    baton->bufferLength = buffer.length;
    baton->bufferData = buffer.buffer;
    baton->timeout = timeout;
    baton->complete = false;

    baton->start();
}