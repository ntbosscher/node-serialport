//
//  util.cpp
//  
//
//  Created by nate bosscher on 2020-11-18.
//

#include "util.h"
#include <chrono>

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

std::ofstream logger(std::string id) {
    std::string fileName = "%HOMEPATH%\\AppData\\Local\\console.log." + id + ".txt";
    return std::ofstream(fileName, std::ofstream::app);
}

v8::Local<v8::Value> getValueFromObject(v8::Local<v8::Object> options, std::string key) {
  v8::Local<v8::String> v8str = Nan::New<v8::String>(key).ToLocalChecked();
  return Nan::Get(options, v8str).ToLocalChecked();
}

int getIntFromObject(v8::Local<v8::Object> options, std::string key) {
  return Nan::To<v8::Int32>(getValueFromObject(options, key)).ToLocalChecked()->Value();
}

bool getBoolFromObject(v8::Local<v8::Object> options, std::string key) {
  return Nan::To<v8::Boolean>(getValueFromObject(options, key)).ToLocalChecked()->Value();
}

v8::Local<v8::String> getStringFromObj(v8::Local<v8::Object> options, std::string key) {
  return Nan::To<v8::String>(getValueFromObject(options, key)).ToLocalChecked();
}

double getDoubleFromObject(v8::Local<v8::Object> options, std::string key) {
  return Nan::To<double>(getValueFromObject(options, key)).FromMaybe(0);
}

int currentMs() {
  return clock() / (CLOCKS_PER_SEC / 1000);
}