
#include "Get.h"
#include "Util.h"

NAN_METHOD(Get) {
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

        GetBaton* baton = new GetBaton();
        baton->fd = fd;
        baton->cts = false;
        baton->dsr = false;
        baton->dcd = false;
        baton->callback.Reset(info[1].As<v8::Function>());

        uv_work_t* req = new uv_work_t();
        req->data = baton;
        uv_queue_work(uv_default_loop(), req, EIO_Get, (uv_after_work_cb)EIO_AfterGet);
}

void EIO_AfterGet(uv_work_t* req) {
    Nan::HandleScope scope;

    GetBaton* data = static_cast<GetBaton*>(req->data);

    v8::Local<v8::Value> argv[2];

    if (data->errorString[0]) {
        argv[0] = v8::Exception::Error(Nan::New<v8::String>(data->errorString).ToLocalChecked());
        argv[1] = Nan::Undefined();
    } else {
        v8::Local<v8::Object> results = Nan::New<v8::Object>();
        Nan::Set(results, Nan::New<v8::String>("cts").ToLocalChecked(), Nan::New<v8::Boolean>(data->cts));
        Nan::Set(results, Nan::New<v8::String>("dsr").ToLocalChecked(), Nan::New<v8::Boolean>(data->dsr));
        Nan::Set(results, Nan::New<v8::String>("dcd").ToLocalChecked(), Nan::New<v8::Boolean>(data->dcd));

        argv[0] = Nan::Null();
        argv[1] = results;
    }

    data->callback.Call(2, argv, data);

    delete data;
    delete req;
}