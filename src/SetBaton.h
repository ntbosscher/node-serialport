#ifndef SetBaton_h
#define SetBaton_h

#include <nan.h>
#include "BatonBase.h"

class SetBaton : public BatonBase
{
public:
    SetBaton(char *name, v8::Local<v8::Function> callback_) : BatonBase(name, callback_)
    {
        request.data = this;
    }

    int fd = 0;
    int result = 0;
    bool rts = false;
    bool cts = false;
    bool dtr = false;
    bool dsr = false;
    bool brk = false;

    v8::Local<v8::Value> getReturnValue() override;
    void run() override;
};

NAN_METHOD(SetParameter);

#endif