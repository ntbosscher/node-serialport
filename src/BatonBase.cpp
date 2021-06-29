#include "./BatonBase.h"

void DoAction(uv_work_t* req) {
    auto baton = static_cast<BatonBase*>(req->data);    
    baton->run();
}

void AfterAction(uv_work_t* req, int status) {
    
    auto baton = static_cast<BatonBase*>(req->data);
    if(status == UV_ECANCELED) {
        strcpy(baton->errorString, "Job cancelled by libuv");
    }
    
    baton->done();
    delete baton;
}

void BatonBase::logVerbose(std::string input) {
    if(!verbose) return;

    auto out = defaultLogger();
    out << std::to_string(currentMs()) << " " << (void*)this << " " << debugName << " " << input << "\n";
    out.close();
}


BatonBase::BatonBase(char* name, v8::Local<v8::Function> callback_): AsyncResource(name), errorString(), debugName(name) {
    debugName = std::string(name);
    callback.Reset(callback_);
    snprintf(errorString, sizeof(errorString), "");
    verbose = verboseLoggingEnabled();
}

void BatonBase::start() {
    request.data = this;
    uv_queue_work(Nan::GetCurrentEventLoop(), &request, DoAction, reinterpret_cast<uv_after_work_cb>(AfterAction));
}

void BatonBase::run() {
    strcpy(errorString, "BatonBase.run should be overridden");
}

v8::Local<v8::Value> BatonBase::getReturnValue() {
    strcpy(errorString, "BatonBase.getReturnValues should be overridden");    
    return Nan::Undefined();
}

void BatonBase::done() {
    
    v8::Local<v8::Function> callback_ = Nan::New(callback);
    v8::Local<v8::Object> target = Nan::New<v8::Object>();

    v8::Local<v8::Value> argv[2];
    
    if(errorString[0]) {
        argv[0] = v8::Exception::Error(Nan::New<v8::String>(errorString).ToLocalChecked());
        argv[1] = Nan::Undefined();
    } else {
        argv[0] = Nan::Null();
        argv[1] = getReturnValue();
    }
    
    runInAsyncScope(target, callback_, 2, argv);
}