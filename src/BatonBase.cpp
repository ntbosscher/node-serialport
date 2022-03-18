#include "./BatonBase.h"

void DoAction(uv_work_t* req) {
    auto baton = static_cast<BatonBase*>(req->data);
    baton->logVerbose("run start");
    baton->run();
    baton->logVerbose("run end");
}

void AfterAction(uv_work_t* req, int status) {
    
    auto baton = static_cast<BatonBase*>(req->data);
    if(status == UV_ECANCELED) {
        strcpy(baton->errorString, "Job cancelled by libuv");
    }
    
    baton->done();
    baton->logVerbose("pre-delete baton");

    delete baton;
}

void BatonBase::logVerbose(std::string input) {
    if(!verbose) return;

    muLogger.lock();

    auto out = defaultLogger();
    out << std::to_string(currentMs()) << " " << (void*)this << " " << debugName << " " << input << "\n";
    out.close();

    muLogger.unlock();
}

BatonBase::BatonBase(std::string name, v8::Local<v8::Function> callback_): AsyncResource(name.c_str()), errorString(), debugName(name) {
    debugName = name;
    callback.Reset(callback_);
    snprintf(errorString, sizeof(errorString), "");
    verbose = verboseLoggingEnabled();

    logVerbose("create");
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

    logVerbose("run callback");
    
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