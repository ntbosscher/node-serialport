#include "./OpenBaton.h"
#include "./V8ArgDecoder.h"
#include <cstring>

v8::Local<v8::Value> OpenBaton::getReturnValue()
{
    this->logVerbose("return");
    return Nan::New<v8::Int32>(this->result);
}

// assumes src is a null terminated string
void strcpy(char* dst, int dstLen, const char* src) {
    for(int i = 0; i < dstLen; i++) {
        dst[i] = src[i];

        if(src[i] == '\0') {
            return;
        }
    }

    // set last character to null it not already done so
    dst[dstLen-1] = '\0';
}

NAN_METHOD(Open)
{
        V8ArgDecoder args(&info);

        auto path = args.takeString();
        auto object = args.takeObject();
        auto cb = args.takeFunction();

        if(args.hasError()) return;

        OpenBaton *baton = new OpenBaton("OpenBaton", cb);
        strcpy(baton->path, sizeof(baton->path), path.c_str());
        baton->baudRate = object.getInt("baudRate");
        baton->dataBits = object.getInt("dataBits");
        baton->parity = ToParityEnum(object.getV8String("parity"));
        baton->stopBits = ToStopBitEnum(object.getDouble("stopBits"));
        baton->rtscts = object.getBool("rtscts");
        baton->xon = object.getBool("xon");
        baton->xoff = object.getBool("xoff");
        baton->xany = object.getBool("xany");
        baton->hupcl = object.getBool("hupcl");
        baton->lock = object.getBool("lock");

        auto watcher = new DeviceWatcher;
        watcher->verbose = baton->verbose;
        watcher->eventsCallback.Reset(object.getFunction("eventsCallback"));
        baton->watcher = watcher;

#ifndef WIN32
        baton->vmin = object.getInt("vmin");
        baton->vtime = object.getInt("vtime");
#endif

        baton->logVerbose("open-start");
        baton->start();
}