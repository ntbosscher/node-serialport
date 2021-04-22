#include "./serialport.h"
#include <fstream>
#include "./util.h"
#include "./WriteBaton.h"
#include "./ReadBaton.h"
#include "./UpdateBaton.h"
#include "./SetBaton.h"
#include "./ListBaton.h"

#ifdef WIN32
  #define strncasecmp strnicmp
  #include "./win.h"
#endif

NAN_METHOD(Close) {
  // file descriptor
  if (!info[0]->IsInt32()) {
    Nan::ThrowTypeError("First argument must be an int");
    return;
  }

  // callback
  if (!info[1]->IsFunction()) {
    Nan::ThrowTypeError("Second argument must be a function");
    return;
  }

  VoidBaton* baton = new VoidBaton();
  baton->fd = Nan::To<v8::Int32>(info[0]).ToLocalChecked()->Value();
  baton->callback.Reset(info[1].As<v8::Function>());

  uv_work_t* req = new uv_work_t();
  req->data = baton;
  uv_queue_work(uv_default_loop(), req, EIO_Close, (uv_after_work_cb)EIO_AfterClose);
}

void EIO_AfterClose(uv_work_t* req) {
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

NAN_METHOD(GetBaudRate) {
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

  GetBaudRateBaton* baton = new GetBaudRateBaton();
  baton->fd = fd;
  baton->baudRate = 0;
  baton->callback.Reset(info[1].As<v8::Function>());

  uv_work_t* req = new uv_work_t();
  req->data = baton;
  uv_queue_work(uv_default_loop(), req, EIO_GetBaudRate, (uv_after_work_cb)EIO_AfterGetBaudRate);
}

void EIO_AfterGetBaudRate(uv_work_t* req) {
  Nan::HandleScope scope;

  GetBaudRateBaton* data = static_cast<GetBaudRateBaton*>(req->data);

  v8::Local<v8::Value> argv[2];

  if (data->errorString[0]) {
    argv[0] = v8::Exception::Error(Nan::New<v8::String>(data->errorString).ToLocalChecked());
    argv[1] = Nan::Undefined();
  } else {
    v8::Local<v8::Object> results = Nan::New<v8::Object>();
    Nan::Set(results, Nan::New<v8::String>("baudRate").ToLocalChecked(), Nan::New<v8::Integer>(data->baudRate));

    argv[0] = Nan::Null();
    argv[1] = results;
  }
  data->callback.Call(2, argv, data);

  delete data;
  delete req;
}

NAN_METHOD(ConfigureLogging) {

    if (!info[0]->IsBoolean())
    {
        Nan::ThrowTypeError("First argument must be a boolean");
        return;
    }

    if (!info[1]->IsString())
    {
        Nan::ThrowTypeError("Second argument must be a string");
        return;
    }

    auto enabled = info[0]->IsTrue();
    Nan::Utf8String path(info[1]);

    configureLogging(enabled, std::string(*path));
}

NAN_METHOD(Drain) {
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

  VoidBaton* baton = new VoidBaton();
  baton->fd = fd;
  baton->callback.Reset(info[1].As<v8::Function>());

  uv_work_t* req = new uv_work_t();
  req->data = baton;
  uv_queue_work(uv_default_loop(), req, EIO_Drain, (uv_after_work_cb)EIO_AfterDrain);
}

void EIO_AfterDrain(uv_work_t* req) {
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

NAN_MODULE_INIT(init) {
    Nan::HandleScope scope;
    Nan::SetMethod(target, "set", Set);
    Nan::SetMethod(target, "get", Get);
    Nan::SetMethod(target, "getBaudRate", GetBaudRate);
    Nan::SetMethod(target, "open", Open);
    Nan::SetMethod(target, "update", Update);
    Nan::SetMethod(target, "close", Close);
    Nan::SetMethod(target, "flush", Flush);
    Nan::SetMethod(target, "drain", Drain);
    Nan::SetMethod(target, "configureLogging", ConfigureLogging);
    Nan::SetMethod(target, "write", Write);
    Nan::SetMethod(target, "read", Read);
    Nan::SetMethod(target, "list", List);
}

NODE_MODULE(serialport, init);
