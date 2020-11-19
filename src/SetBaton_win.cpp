
#include "SetBaton.h"
#include <nan.h>
#include "win.h"

v8::Local<v8::Value> SetBaton::getReturnValue()
{
    return Nan::Null();
}

void SetBaton::run()
{

    if (this->rts)
    {
        EscapeCommFunction(int2handle(this->fd), SETRTS);
    }
    else
    {
        EscapeCommFunction(int2handle(this->fd), CLRRTS);
    }

    if (this->dtr)
    {
        EscapeCommFunction(int2handle(this->fd), SETDTR);
    }
    else
    {
        EscapeCommFunction(int2handle(this->fd), CLRDTR);
    }

    if (this->brk)
    {
        EscapeCommFunction(int2handle(this->fd), SETBREAK);
    }
    else
    {
        EscapeCommFunction(int2handle(this->fd), CLRBREAK);
    }

    DWORD bits = 0;

    GetCommMask(int2handle(this->fd), &bits);

    bits &= ~(EV_CTS | EV_DSR);

    if (this->cts)
    {
        bits |= EV_CTS;
    }

    if (this->dsr)
    {
        bits |= EV_DSR;
    }

    if (!SetCommMask(int2handle(this->fd), bits))
    {
        ErrorCodeToString("Setting options on COM port (SetCommMask)", GetLastError(), this->errorString);
        return;
    }
}

NAN_METHOD(Set)
{
    if (!info[0]->IsInt32())
    {
        Nan::ThrowTypeError("First argument must be an int");
        return;
    }
    int fd = Nan::To<int>(info[0]).FromJust();

    // options
    if (!info[1]->IsObject())
    {
        Nan::ThrowTypeError("Second argument must be an object");
        return;
    }
    v8::Local<v8::Object> options = Nan::To<v8::Object>(info[1]).ToLocalChecked();

    // callback
    if (!info[2]->IsFunction())
    {
        Nan::ThrowTypeError("Third argument must be a function");
        return;
    }
    v8::Local<v8::Function> callback = info[2].As<v8::Function>();

    auto cb = Nan::To<v8::Function>(info[2]).ToLocalChecked();
    SetBaton *baton = new SetBaton("SetBaton", cb);

    baton->fd = fd;
    baton->brk = getBoolFromObject(options, "brk");
    baton->rts = getBoolFromObject(options, "rts");
    baton->cts = getBoolFromObject(options, "cts");
    baton->dtr = getBoolFromObject(options, "dtr");
    baton->dsr = getBoolFromObject(options, "dsr");

    baton->start();
}