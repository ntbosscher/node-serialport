//
//  util.cpp
//  
//
//  Created by nate bosscher on 2020-11-18.
//

#include "util.h"

char *copySubstring(char *someString, int n) {
    char *new_ = reinterpret_cast<char*>(malloc(sizeof(char)*n + 1));
    strncpy_s(new_, n + 1, someString, n);
    new_[n] = '\0';
    return new_;
}

void setIfNotEmpty(v8::Local<v8::Object> item, std::string key, const char *value) {
    v8::Local<v8::String> v8key = Nan::New<v8::String>(key).ToLocalChecked();
    if (strlen(value) > 0) {
        Nan::Set(item, v8key, Nan::New<v8::String>(value).ToLocalChecked());
    } else {
        Nan::Set(item, v8key, Nan::Undefined());
    }
}