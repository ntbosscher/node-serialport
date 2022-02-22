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
#include "./OpenBaton.h"
#include "./ConfigureLogging.h"
#include "./BufferedReadBaton.h"

#include <node.h>

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
        Nan::SetMethod(target, "bufferedRead", BufferedRead);
}

NAN_MODULE_WORKER_ENABLED(serialport, init);

// using namespace v8;

// class AddonData {
//  public:
//   AddonData(Isolate* isolate, Local<Object> exports):
//       call_count(0) {
//     // Link the existence of this object instance to the existence of exports.
//     exports_.Reset(isolate, exports);
//     exports_.SetWeak(this, DeleteMe, v8::WeakCallbackType::kParameter);
//   }

//   ~AddonData() {
//     if (!exports_.IsEmpty()) {
//       // Reset the reference to avoid leaking data.
//       exports_.ClearWeak();
//       exports_.Reset();
//     }
//   }

//   // Per-addon data.
//   int call_count;

//  private:
//   // Method to call when "exports" is about to be garbage-collected.
//   static void DeleteMe(const v8::WeakCallbackInfo<AddonData>& info) {
//     delete info.GetParameter();
//   }

//   // Weak handle to the "exports" object. An instance of this class will be
//   // destroyed along with the exports object to which it is weakly bound.
//   v8::Persistent<v8::Object> exports_;
// };

// static void Method(const v8::FunctionCallbackInfo<v8::Value>& info) {
//   // Retrieve the per-addon-instance data.
//   AddonData* data =
//       reinterpret_cast<AddonData*>(info.Data().As<External>()->Value());
//   data->call_count++;
//   info.GetReturnValue().Set((double)data->call_count);
// }

// #ifdef NODE_MODULE_INIT
// NODE_MODULE_INIT() {
//   Isolate* isolate = context->GetIsolate();

//   // Create a new instance of AddonData for this instance of the addon.
//   AddonData* data = new AddonData(isolate, exports);
//   // Wrap the data in a v8::External so we can pass it to the method we expose.
//   Local<External> external = External::New(isolate, data);

//   // Expose the method "Method" to JavaScript, and make sure it receives the
//   // per-addon-instance data we created above by passing `external` as the
//   // third parameter to the FunctionTemplate constructor.
//   exports->Set(context,
//                String::NewFromUtf8(isolate, "method", NewStringType::kNormal)
//                   .ToLocalChecked(),
//                FunctionTemplate::New(isolate, Method, external)
//                   ->GetFunction(context).ToLocalChecked()).FromJust();
// }
// #else
// NAN_MODULE_INIT(Init) {
//   Isolate* isolate = context->GetIsolate();

//   // Create a new instance of AddonData for this instance of the addon.
//   AddonData* data = new AddonData(isolate, exports);
//   // Wrap the data in a v8::External so we can pass it to the method we expose.
//   Local<External> external = External::New(isolate, data);

//   // Expose the method "Method" to JavaScript, and make sure it receives the
//   // per-addon-instance data we created above by passing `external` as the
//   // third parameter to the FunctionTemplate constructor.
//   exports->Set(context,
//                String::NewFromUtf8(isolate, "method", NewStringType::kNormal)
//                   .ToLocalChecked(),
//                FunctionTemplate::New(isolate, Method, external)
//                   ->GetFunction(context).ToLocalChecked()).FromJust();
// }

// NODE_MODULE(serialport, Init)
// #endif