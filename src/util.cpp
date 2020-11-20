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
    std::string fileName = "C:\\Users\\Nate\\AppData\\Local\\console.log." + id + ".txt";
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

std::string wStr2Char(wchar_t *buf)
{
    // std::wstring wstr(buf);
    // std::string cstr(wstr.begin(), wstr.end());

    char* out = (char*)malloc(wcslen(buf)+1);
    sprintf(out, "%ls", buf);

    auto str = std::string(out);
    free(out);

    return str;
}

char* guid2Str(const GUID *id, char *out)
{
    int i;
    char *ret = out;
    out += sprintf(out, "%.8lX-%.4hX-%.4hX-", id->Data1, id->Data2, id->Data3);
    for (i = 0; i < sizeof(id->Data4); ++i)
    {
        out += sprintf(out, "%.2hhX", id->Data4[i]);
        if (i == 1)
            *(out++) = '-';
    }
    return ret;
}