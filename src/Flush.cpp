
#include "Flush.h"
#include "./VoidBaton.h"

NAN_METHOD(Flush) {
    // file descriptor
    if (!info[0]->IsInt32()) {
        Nan::ThrowTypeError("First argument must be an int");
        return;
    }
    int fd = Nan::To<int>(info[0]).FromJust();

    // callback
    if (!info[1]->IsFunction()) {
        Nan::ThrowTypeError("Second argument must be a function");
        return;
    }
    v8::Local<v8::Function> callback = info[1].As<v8::Function>();

    VoidBaton* baton = new VoidBaton();
    baton->fd = fd;
    baton->callback.Reset(callback);

    uv_work_t* req = new uv_work_t();
    req->data = baton;
    uv_queue_work(uv_default_loop(), req, EIO_Flush, (uv_after_work_cb)EIO_AfterFlush);
}

void EIO_AfterFlush(uv_work_t* req) {
    Nan::HandleScope scope;

    VoidBaton* data = static_cast<VoidBaton*>(req->data);

    v8::Local<v8::Value> argv[1];

    if (data->errorString[0]) {
        argv[0] = v8::Exception::Error(Nan::New<v8::String>(data->errorString).ToLocalChecked());
    } else {
        argv[0] = Nan::Null();
    }

    data->callback.Call(1, argv, data);

    delete data;
    delete req;
}