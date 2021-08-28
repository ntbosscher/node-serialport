#ifndef BufferedReadBaton_h
#define BufferedReadBaton_h

#include <nan.h>
#include "BatonBase.h"
#include <thread>
#include <condition_variable>
#include <mutex>
#include <queue>

struct BufferItem {
    char* buffer;
    int length;
};

class BufferedReadBaton : public BatonBase {
public:
    BufferedReadBaton(char *name, v8::Local<v8::Function> callback_) : BatonBase(name, callback_)
    {
        request.data = this;
    }

    int fd = 0;
    int noDataTimeoutMs;
    Nan::Persistent<v8::Function> onDataCallback;
    
    std::mutex syncMutex;
    std::condition_variable signal;
    std::queue<BufferItem*> queue;
    bool readThreadIsRunning = false;

    v8::Local<v8::Value> getReturnValue() override;
    void run() override;
    void push(char* buffer, int length);
    void sendData(char* buffer, int length);
};

NAN_METHOD(BuferedRead);

#endif