
#include <v8.h>
//#include "./SerialPort.h"
#include "./ListBaton.h"
#include "./Util.h"

void EIO_Close(uv_work_t* req) {}
void EIO_Flush(uv_work_t* req) {}
void EIO_Get(uv_work_t* req) {}
void EIO_GetBaudRate(uv_work_t* req) {}
void EIO_Drain(uv_work_t* req) {}

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

NAN_MODULE_INIT(init) {
        Nan::HandleScope scope;
//        Nan::SetMethod(target, "set", Set);
//        Nan::SetMethod(target, "get", Get);
//        Nan::SetMethod(target, "getBaudRate", GetBaudRate);
//        Nan::SetMethod(target, "open", Open);
//        Nan::SetMethod(target, "update", Update);
//        Nan::SetMethod(target, "close", Close);
//        Nan::SetMethod(target, "flush", Flush);
//        Nan::SetMethod(target, "drain", Drain);
        Nan::SetMethod(target, "configureLogging", ConfigureLogging);
//        Nan::SetMethod(target, "write", Write);
//        Nan::SetMethod(target, "read", Read);
        Nan::SetMethod(target, "list", List);
}

NODE_MODULE(serialport, init);
