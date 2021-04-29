
#ifndef UpdateBaton_h
#define UpdateBaton_h

#include <nan.h>
#include "./Util.h"
#include "BatonBase.h"

class UpdateBaton : public BatonBase
{
public:
    UpdateBaton(char *name, v8::Local<v8::Function> callback_) : BatonBase(name, callback_)
    {
        request.data = this;
    }

    int fd = 0;
    int baudRate = 0;

    void run() override;
    v8::Local<v8::Value> getReturnValue() override;
};

NAN_METHOD(Update);

#endif