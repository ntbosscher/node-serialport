
#include "./ConfigureLogging.h"
#include "./Util.h"

NAN_METHOD(ConfigureLogging) {

    if (!info[0]->IsBoolean())
    {
        Nan::ThrowTypeError("First argument must be a boolean");
        return;
    }

    if (!info[1]->IsString())
    {
        Nan::ThrowTypeError("Second argument must be a string");
        return;
    }

    auto enabled = info[0]->IsTrue();
    Nan::Utf8String path(info[1]);

    configureLogging(enabled, std::string(*path));
}