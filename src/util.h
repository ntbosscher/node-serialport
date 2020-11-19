//
//  util.hpp
//  
//
//  Created by nate bosscher on 2020-11-18.
//

#ifndef util_h
#define util_h

#include <stdio.h>
#include <v8.h>

#define TIMEOUT_PRECISION 10
#define ERROR_STRING_SIZE 1024
#define ARRAY_SIZE(arr)     (sizeof(arr)/sizeof(arr[0]))

void ErrorCodeToString(const char* prefix, int errorCode, char *errorStr);
std::ofstream logger(std::string id);

void setIfNotEmpty(v8::Local<v8::Object> item, std::string key, const char *value) {
    v8::Local<v8::String> v8key = Nan::New<v8::String>(key).ToLocalChecked();
    if (strlen(value) > 0) {
        Nan::Set(item, v8key, Nan::New<v8::String>(value).ToLocalChecked());
    } else {
        Nan::Set(item, v8key, Nan::Undefined());
    }
}

char *copySubstring(char *someString, int n) {
    char *new_ = reinterpret_cast<char*>(malloc(sizeof(char)*n + 1));
    strncpy_s(new_, n + 1, someString, n);
    new_[n] = '\0';
    return new_;
}

#endif /* util_hpp */
