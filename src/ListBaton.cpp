
#include "./ListBaton.h"
#include <nan.h>

v8::Local<v8::Value> ListBaton::getReturnValue()
{
    v8::Local<v8::Array> results = Nan::New<v8::Array>();
    int i = 0;

    for (std::list<ListResultItem *>::iterator it = this->results.begin(); it != this->results.end(); ++it, i++)
    {
        v8::Local<v8::Object> item = Nan::New<v8::Object>();

        setIfNotEmpty(item, "path", (*it)->path.c_str());
        setIfNotEmpty(item, "manufacturer", (*it)->manufacturer.c_str());
        setIfNotEmpty(item, "serialNumber", (*it)->serialNumber.c_str());
        setIfNotEmpty(item, "pnpId", (*it)->pnpId.c_str());
        setIfNotEmpty(item, "locationId", (*it)->locationId.c_str());
        setIfNotEmpty(item, "vendorId", (*it)->vendorId.c_str());
        setIfNotEmpty(item, "productId", (*it)->productId.c_str());

        Nan::Set(results, i, item);
    }

    for (std::list<ListResultItem *>::iterator it = this->results.begin(); it != this->results.end(); ++it)
    {
        delete *it;
    }

    return results;
}

NAN_METHOD(List)
{
    V8ArgDecoder args(&info);
    auto cb = args.takeFunction();

    if(args.hasError()) return;

    ListBaton *baton = new ListBaton(cb);
    baton->start();
}