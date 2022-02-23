#ifndef ReadBaton_h
#define ReadBaton_h

#include <nan.h>
#include "BatonBase.h"

class ReadBaton : public BatonBase {
public:
    ReadBaton(std::string name, v8::Local<v8::Function> callback_) : BatonBase(name, callback_)
    {
        request.data = this;
    }

    int fd = 0;
    int timeout = -1;
    char* bufferData = nullptr;
    size_t bufferLength = 0;
    size_t bytesRead = 0;
    size_t bytesToRead = 0;
    size_t offset = 0;
    bool complete = false;

    v8::Local<v8::Value> getReturnValue() override;
    void run() override;
};

int readFromSerial(int fd, char* buffer, int length, bool blocking, char* error);
NAN_METHOD(Read);

#endif