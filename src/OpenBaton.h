
#ifndef openbaton_h
#define openbaton_h

#include <nan.h>
#include "Util.h"
#include "BatonBase.h"

SerialPortParity ToParityEnum(const v8::Local<v8::String>& str);
SerialPortStopBits ToStopBitEnum(double stopBits);

SerialPortParity NAN_INLINE(ToParityEnum(const v8::Local<v8::String> &v8str))
{
    Nan::HandleScope scope;
    Nan::Utf8String str(v8str);
    size_t count = strlen(*str);
    SerialPortParity parity = SERIALPORT_PARITY_NONE;
    if (!strncasecmp(*str, "none", count))
    {
        parity = SERIALPORT_PARITY_NONE;
    }
    else if (!strncasecmp(*str, "even", count))
    {
        parity = SERIALPORT_PARITY_EVEN;
    }
    else if (!strncasecmp(*str, "mark", count))
    {
        parity = SERIALPORT_PARITY_MARK;
    }
    else if (!strncasecmp(*str, "odd", count))
    {
        parity = SERIALPORT_PARITY_ODD;
    }
    else if (!strncasecmp(*str, "space", count))
    {
        parity = SERIALPORT_PARITY_SPACE;
    }
    return parity;
}

SerialPortStopBits NAN_INLINE(ToStopBitEnum(double stopBits))
{
    if (stopBits > 1.4 && stopBits < 1.6)
    {
        return SERIALPORT_STOPBITS_ONE_FIVE;
    }
    if (stopBits == 2)
    {
        return SERIALPORT_STOPBITS_TWO;
    }
    return SERIALPORT_STOPBITS_ONE;
}

class OpenBaton : public BatonBase
{
public:
    OpenBaton(char *name, v8::Local<v8::Function> callback_) : BatonBase(name, callback_), path()
    {
        request.data = this;
    }

    char path[1024];
    int fd = 0;
    int result = 0;
    int baudRate = 0;
    int dataBits = 0;
    bool rtscts = false;
    bool xon = false;
    bool xoff = false;
    bool xany = false;
    bool dsrdtr = false;
    bool hupcl = false;
    bool lock = false;

    SerialPortParity parity;
    SerialPortStopBits stopBits;

    DeviceWatcher *watcher;
    
#ifndef WIN32
    uint8_t vmin = 0;
    uint8_t vtime = 0;
#endif

    void run() override;
    v8::Local<v8::Value> getReturnValue() override;
};

NAN_METHOD(Open);

#endif