//
//  BatonBase.hpp
//  
//
//  Created by nate bosscher on 2020-11-18.
//

#ifndef BatonBase_hpp
#define BatonBase_hpp

#include <stdio.h>
#include <v8.h>

void DoAction(uv_work_t* req) {
    auto baton = static_cast<BatonBase*>(req->data);
    
    if(baton->verbose) {
        auto out = logger("baton-base-logger");
        out << baton->debugName << " run job\n";
        out.close();
    }
    
    details->run();
}

void AfterAction(uv_work_t* req, int status) {
    
    auto baton = static_cast<BatonBase*>(req->data);
    if(status == UV_ECANCELED) {
        strcpy(baton->errorString, "Job cancelled by libuv");
    }
    
    if(baton->verbose) {
        auto out = logger("baton-base-logger");
        out << baton->debugName << " after job\n";
        out.close();
    }
    
    baton->done();
}

class BatonBase: public Nan::AsyncResource {
public:
    Nan::Callback callback;
    char errorString[ERROR_STRING_SIZE];
    uv_work_t request;
    bool done;
    
    string debugName;
    bool verbose = false;
    
    BatonBase(char* name, v8::Local<v8::Function> callback_): AsyncResource(name), errorString(), debugName(name) {
        
        callback.Reset(callback_);
        snprintf(errorString, sizeof(errorString), "");
    }
    
    start() {
        if(verbose) {
            auto out = logger("baton-base-logger");
            out << debugName << " queue job\n";
            out.close();
        }
        
        uv_queue_work(Nan::GetCurrentEventLoop(), &baton->request, DoAction, reinterpret_cast<uv_after_work_cb>(AfterAction));
    }
    
    virtual void run() {
        strcpy(errorString, "BatonBase.run should be overridden");
        done = true;
    }
    
    virtual v8::Local<v8::Value> getReturnValue() {
        strcpy(errorString, "BatonBase.getReturnValues should be overridden");
        done = true;
        
        return nullptr;
    }
    
    void done() {
        if(verbose) {
            auto out = logger("baton-base-logger");
            out << debugName << " formatting response\n";
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
        
        runInAsyncScope(target, callback, 2, argv);
        
        if(verbose) {
            auto out = logger("baton-base-logger");
            out << debugName << " sent-to-js\n";
            out.close();
        }
        
        delete data;
    }
    
    ~BatonBase() {
      callback.Reset();
    }
}

/*
class ActionBaton : public BatonBase {
    ActionBaton(char* name, v8::Local<v8::Function> callback_): BatonBase() {
        request.data = this;
    }
    
    override v8::Local<v8::Value> getReturnValue() {
        // format return value
    }
    
    override void run() {
        // do run stuff
    }
}

NAN_METHOD(Action) {
    // callback
    if (!info[0]->IsFunction()) {
        Nan::ThrowTypeError("First argument must be a function");
        return;
    }

    auto cb = Nan::To<v8::Function>(info[0]).ToLocalChecked();

    ListBaton* baton = new BatonBase("BatonBase", cb);
    baton->start();
}
*/

#endif /* BatonBase_hpp */
