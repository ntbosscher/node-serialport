#include "./BatonBase.h";

void DoAction(uv_work_t* req) {
    auto baton = static_cast<BatonBase*>(req->data);
    
    if(baton->verbose) {
        auto out = defaultLogger();
        out << currentMs() << " " << baton->debugName << " run job\n";
        out.close();
    }
    
    baton->run();

    if(baton->verbose) {
        auto out = defaultLogger();
        out << currentMs() << " " << baton->debugName << " run finished\n";
        out.close();
    }
}

void AfterAction(uv_work_t* req, int status) {
    
    auto baton = static_cast<BatonBase*>(req->data);
    if(status == UV_ECANCELED) {
        strcpy(baton->errorString, "Job cancelled by libuv");
    }
    
    if(baton->verbose) {
        auto out = defaultLogger();
        out << currentMs() << " " << baton->debugName << " after job\n";
        out.close();
    }
    
    baton->done();
    delete baton;
}

void BatonBase::logVerbose(std::string input) {
    if(!verbose) return;

    auto out = defaultLogger();
    out << std::to_string(currentMs()) << " " << debugName << " " << input << "\n";
    out.close();
}


BatonBase::BatonBase(char* name, v8::Local<v8::Function> callback_): AsyncResource(name), errorString(), debugName(name) {
    debugName = std::string(name);
    callback.Reset(callback_);
    snprintf(errorString, sizeof(errorString), "");
    verbose = verboseLoggingEnabled();
}

void BatonBase::start() {
    if(verbose) {
        auto out = defaultLogger();
        out << currentMs() << " " << debugName << " queue job\n";
        out.close();
    }

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
    if(verbose) {
        auto out = defaultLogger();
        out << currentMs() << " " << debugName << " formatting response\n";
        out.close();
    }
    
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
    
    if(verbose) {
        auto out = defaultLogger();
        out << currentMs() << " " << debugName << " sent-to-js\n";
        out.close();
    }
}