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

class BufferedReadBaton {
public:
    BufferedReadBaton()
    {}

    int fd = 0;
    int noDataTimeoutMs;

    char errorString[ERROR_STRING_SIZE];
    
    Nan::Persistent<v8::Function> onData;
    Nan::Persistent<v8::Function> onDone;
    
    std::mutex syncMutex;
    std::condition_variable signal;
    std::queue<BufferItem*> queue;
    bool readThreadIsRunning = false;
    
    void push(char* buffer, int length);
};

NAN_METHOD(BufferedRead);

#endif