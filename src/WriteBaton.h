#ifndef WriteBaton_h
#define WriteBaton_h

#include <nan.h>
#include "BatonBase.h"

class WriteBaton : public BatonBase {
private:
    void WriteBaton::writeEcho(int deadline);
    void WriteBaton::writeNormal(int deadline);
public:
    WriteBaton(char *name, v8::Local<v8::Function> callback_) : BatonBase(name, callback_)
    {
        request.data = this;
    }

    int fd = 0;
    int timeout = -1;
    char *bufferData = nullptr;
    size_t bufferLength = 0;
    size_t offset = 0;
    size_t bytesWritten = 0;
    bool complete = false;
    Nan::Persistent<v8::Object> buffer;
    int result = 0;
    bool echoMode = false;

    v8::Local<v8::Value> getReturnValue() override;
    void run() override;
};

NAN_METHOD(Write);

#endif