
#ifndef ListBaton_h
#define ListBaton_h

#include <nan.h>
#include "BatonBase.h"
#include "./V8ArgDecoder.h"
#include <list>

struct ListResultItem {
    std::string path;
    std::string manufacturer;
    std::string serialNumber;
    std::string pnpId;
    std::string locationId;
    std::string vendorId;
    std::string productId;
};

class ListBaton : public BatonBase {
public:
    std::list<ListResultItem *> results;

    ListBaton(v8::Local<v8::Function> callback_) : BatonBase("node-serialport:ListBaton", callback_)
    {
        logVerbose("ListBaton::constructor");
    }

    v8::Local<v8::Value> getReturnValue() override;
    void run() override;
};

NAN_METHOD(List);

#endif