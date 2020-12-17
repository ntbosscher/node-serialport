
#include "SetBaton.h"
#include <nan.h>
#include "win.h"
#include "./V8ArgDecoder.h";

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
    V8ArgDecoder args(&info);

    auto fd = args.takeInt32();
    auto object = args.takeObject();
    auto cb = args.takeFunction();

    if(args.hasError()) return;

    SetBaton *baton = new SetBaton("SetBaton", cb);

    baton->fd = fd;
    baton->brk = object.getBool("brk");
    baton->rts = object.getBool("rts");
    baton->cts = object.getBool("cts");
    baton->dtr = object.getBool("dtr");
    baton->dsr = object.getBool("dsr");

    baton->start();
}