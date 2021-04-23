#include "./SerialPort.h"
#include "./ListBaton.h"
#include "./SetBaton.h"
#include "./GetBaudRate.h"
#include "./Get.h"
#include "./UpdateBaton.h"
#include "./WriteBaton.h"
#include "./ReadBaton.h"
#include "./ListBaton.h"
#include "./Util.h"
#include "./Close.h"
#include "./Drain.h"
#include "./Flush.h"

NAN_MODULE_INIT(init) {
        Nan::HandleScope scope;
        Nan::SetMethod(target, "set", SetParameter);
        Nan::SetMethod(target, "get", Get);
        Nan::SetMethod(target, "getBaudRate", GetBaudRate);
        Nan::SetMethod(target, "open", Open);
        Nan::SetMethod(target, "update", Update);
        Nan::SetMethod(target, "close", Close);
        Nan::SetMethod(target, "flush", Flush);
        Nan::SetMethod(target, "drain", Drain);
        Nan::SetMethod(target, "configureLogging", ConfigureLogging);
        Nan::SetMethod(target, "write", Write);
        Nan::SetMethod(target, "read", Read);
        Nan::SetMethod(target, "list", List);
}

NODE_MODULE(serialport, init);
