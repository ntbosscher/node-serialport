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
#include <string>
#include <nan.h>
#include <fstream>
#include "./Util.h"

void DoAction(uv_work_t* req);
void AfterAction(uv_work_t* req, int status);

class BatonBase: public Nan::AsyncResource {
public:
    Nan::Persistent<v8::Function> callback;
    char errorString[ERROR_STRING_SIZE];
    uv_work_t request;
    
    std::string debugName;
    bool verbose = true;
    bool isSingleResult = false;
    
    BatonBase(std::string name, v8::Local<v8::Function> callback_);
    
    void start();
    void done();

    virtual void run();
    virtual v8::Local<v8::Value> getReturnValue();

    void logVerbose(std::string value);
    
    virtual ~BatonBase() {
        logVerbose("destroy");
        callback.Reset();
    }

    virtual v8::Local<v8::Function> getCallback() {
        v8::Local<v8::Function> cb = Nan::New(callback);
        return cb;
    }

    virtual void afterCallback() {}
};

/*
class ActionBaton : public BatonBase {
    ActionBaton(char* name, v8::Local<v8::Function> callback_): BatonBase() {
        request.data = this;
    }
    
    v8::Local<v8::Value> getReturnValue() override {
        // format return value
    }
    
    void run() override {
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
