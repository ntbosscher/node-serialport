
#include "./UpdateBaton.h"
#include "./util.h"
#include "./win.h"

v8::Local<v8::Value> UpdateBaton::getReturnValue()
{
    return Nan::Null();
}

void UpdateBaton::run()
{

    DCB dcb = {0};
    SecureZeroMemory(&dcb, sizeof(DCB));
    dcb.DCBlength = sizeof(DCB);

    if (!GetCommState(int2handle(this->fd), &dcb))
    {
        ErrorCodeToString("Update (GetCommState)", GetLastError(), this->errorString);
        return;
    }

    dcb.BaudRate = this->baudRate;

    if (!SetCommState(int2handle(this->fd), &dcb))
    {
        ErrorCodeToString("Update (SetCommState)", GetLastError(), this->errorString);
        return;
    }
}

NAN_METHOD(Update)
{
    // file descriptor
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

    if (!Nan::Has(options, Nan::New<v8::String>("baudRate").ToLocalChecked()).FromMaybe(false))
    {
        Nan::ThrowTypeError("\"baudRate\" must be set on options object");
        return;
    }

    // callback
    if (!info[2]->IsFunction())
    {
        Nan::ThrowTypeError("Third argument must be a function");
        return;
    }

    auto cb = Nan::To<v8::Function>(info[2]).ToLocalChecked();

    UpdateBaton *baton = new UpdateBaton("UpdateBaton", cb);
    baton->fd = fd;
    baton->baudRate = getIntFromObject(options, "baudRate");

    baton->start();
}