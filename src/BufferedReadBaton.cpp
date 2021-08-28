
#include "./BufferedReadBaton.h"
#include "./V8ArgDecoder.h"
#include <algorithm>
#include "./ReadBaton.h"
#include <Windows.h>
#include <chrono>
#include <thread>

v8::Local<v8::Value> BufferedReadBaton::getReturnValue()
{
    return Nan::New<v8::Integer>(static_cast<int>(0));
}

void reader(BufferedReadBaton* baton) {

    int length = 10000;
    int highWater = 9000;
    int lowWater = 8000;

    char* buffer = (char*)malloc(sizeof(char) * length);
    char errorString[ERROR_STRING_SIZE];
    
    int noDataDeadlineMs = 10 * 1000;
    int received = 0;
    int noDataForMs = 0;
    long totalCount = 0;

    for(;;) {
        
        if(noDataForMs > noDataDeadlineMs) {
            break;
        }

        auto got = readFromSerial(baton->fd, (char*)(buffer + received), length - received, false, errorString);
        if(got == 0) {
            if(received > lowWater) {
                baton->push(buffer, received);
                buffer = (char*)malloc(sizeof(char) * length);
            } else {
                std::this_thread::sleep_for (std::chrono::milliseconds(1));
                noDataForMs += 1;
            }

            continue;
        }
        
        if(got < 0) {
            std::this_thread::sleep_for (std::chrono::milliseconds(1));
            noDataForMs += 1;

            free(buffer);
            
            {
                std::unique_lock<std::mutex> lck(baton->syncMutex);
        
                strcpy(errorString, baton->errorString);
                baton->readThreadIsRunning = false;

                baton->signal.notify_one();
            }
            return;
        }

        totalCount += got;
        received += got;

        if(received > highWater) {
            baton->push(buffer, received);
            buffer = (char*)malloc(sizeof(char) * length);
        }

        noDataDeadlineMs = baton->noDataTimeoutMs;
    }

    if(received > 0) {
        baton->push(buffer, received);
    } else {
        free(buffer);
    }

    {
        muLogger.lock();
        auto out = defaultLogger();
        out << "BufferedReadBaton::reader finished, got " << totalCount << " bytes\n";
        out.close();
        muLogger.unlock();
    }

    {
        std::unique_lock<std::mutex> lck(baton->syncMutex);
        baton->readThreadIsRunning = false;
        baton->signal.notify_one();
    }
}

void BufferedReadBaton::run()
{
    for(;;) 
    {
        char* buffer;
        int length;

        {
            std::unique_lock<std::mutex> lck(syncMutex);

            while(queue.empty() && readThreadIsRunning) signal.wait(lck);
            if(queue.empty()) break;

            auto next = queue.front();
            buffer = next->buffer;
            length = next->length;

            queue.pop();
        }

        sendData(buffer, length);
    }
}

void BufferedReadBaton::push(char* buffer, int length) {
    std::unique_lock<std::mutex> lck(syncMutex);

    auto item = new BufferItem;
    item->buffer = buffer;
    item->length = length;
    queue.push(item);

    signal.notify_one();
}

void BufferedReadBaton::sendData(char* buffer, int length) {
    muLogger.lock();
    auto out = defaultLogger();
    auto hex = bufferToHex(buffer, length);

    out << hex << "\n";
    out.close();
    muLogger.unlock();
    
    free(buffer);
}


NAN_METHOD(BuferedRead)
{
    V8ArgDecoder args(&info);

    auto fd = args.takeInt32();
    auto noDataTimeoutMs = args.takeInt32();
    auto cb = args.takeFunction();
    auto done = args.takeFunction();

    if(args.hasError()) return;

    
    BufferedReadBaton *baton = new BufferedReadBaton("buffered-read-baton", done);

    baton->fd = fd;
    baton->noDataTimeoutMs = noDataTimeoutMs;
    baton->onDataCallback.Reset(cb);
    baton->readThreadIsRunning = true;
    
    std::thread t1(reader, baton);
    baton->start();
}
