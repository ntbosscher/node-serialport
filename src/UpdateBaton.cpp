
#include "./UpdateBaton.h"
#include "./V8ArgDecoder.h"

v8::Local<v8::Value> UpdateBaton::getReturnValue()
{
    return Nan::Null();
}

NAN_METHOD(Update)
{
    V8ArgDecoder args(&info);

    auto fd = args.takeInt32();
    auto object = args.takeObject();
    auto cb = args.takeFunction();

    if(args.hasError()) return;

    if (!object.hasKey("baudRate"))
    {
        Nan::ThrowTypeError("\"baudRate\" must be set on options object");
        return;
    }

    UpdateBaton *baton = new UpdateBaton("UpdateBaton", cb);
    baton->fd = fd;
    baton->baudRate = object.getInt("baudRate");

    baton->start();
}