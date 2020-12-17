
#include "WriteBaton.h"
#include <nan.h>
#include "win.h"
#include <v8.h>
#include "./ReadBaton.h"
#include "./V8ArgDecoder.h"
#include <sstream>
#include <iomanip>

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

string char2hex(char c)
{
  std::ostringstream oss (std::ostringstream::out);
  oss << std::setw(2) << std::setfill('0') << std::hex << (((int)c) & 0xff);
  return oss.str();
}

void WriteBaton::writeEcho(int deadline) {
    if(verbose) {
        auto out = defaultLogger();
        out << currentMs() << " " << debugName << " to-write=" << (bufferLength - offset) << " blocking=" << true << " echoMode=" << echoMode << "\n";
        out << currentMs() << " " << debugName << " buffer-contents: " << bufferToHex(bufferData, (bufferLength - offset)) << "\n";
        out.close();
    }

    char readBuffer[1];
    char writeBuffer[1];

    for(int i = 0; i < bufferLength; i++) {
        writeBuffer[0] = bufferData[i];
        int len = 1;
        int result = 0;
        
        do {
            result = writeToSerial(fd, writeBuffer, len, true, errorString);
            if(result < 0) {
                complete = true;
                return;
            }
            
            if(currentMs() > deadline) {
                logVerbose("timeout " + std::to_string(deadline));
                return;
            }

        } while(result != len);

        logVerbose("wrote " + char2hex(writeBuffer[0]) + " index=" + std::to_string(i));

        do {
            result = readFromSerial(fd, readBuffer, len, true, errorString);
            if(result < 0) {
                complete = true;
                return;
            }

            if(currentMs() > deadline) {
                logVerbose("timeout " + std::to_string(deadline));
                return;
            }

            if(result == 1) {
                if(readBuffer[0] != writeBuffer[0]) {
                    result = 0;
                    logVerbose("read unexpected value " + char2hex(readBuffer[0]));
                } else {
                    logVerbose("read " + char2hex(readBuffer[0]));
                }
            }

        } while(result != len);
    }
    
    logVerbose("complete");
    complete = true;
}

void WriteBaton::writeNormal(int deadline) {
    do
    {
        if(verbose) {
            auto out = defaultLogger();
            out << currentMs() << " " << debugName << " to-write=" << (bufferLength - offset) << " blocking=" << true << " echoMode=" << echoMode << "\n";
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
            out << currentMs() << " " << debugName << " wrote-len=" << bytesTransferred << " \n";
            out.close();
        }

        offset += bytesTransferred;
        complete = offset == bufferLength;

    } while (!complete && currentMs() > deadline);
}

void WriteBaton::run()
{

    int start = currentMs();
    int deadline = start + this->timeout;

    if(echoMode) {
        this->writeEcho(deadline);
    } else {
        this->writeNormal(deadline);
    }

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
    auto echoMode = args.takeBool();
    auto cb = args.takeFunction();

    if(args.hasError()) return;

    WriteBaton *baton = new WriteBaton("write-baton", cb);
    
    baton->fd = fd;
    baton->timeout = timeout;
    baton->echoMode = echoMode;
    baton->bufferData = buffer.buffer;
    baton->bufferLength = buffer.length;
    baton->offset = 0;
    baton->complete = false;

    baton->start();
}