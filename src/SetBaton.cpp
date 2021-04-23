
#include "./SetBaton.h"
#include "./V8ArgDecoder.h"

NAN_METHOD(SetParameter)
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

v8::Local<v8::Value> SetBaton::getReturnValue()
{
    return Nan::Null();
}