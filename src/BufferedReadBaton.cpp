
#include "./BufferedReadBaton.h"
#include "./V8ArgDecoder.h"
#include <algorithm>
#include "./ReadBaton.h"

#ifdef WIN32
#include <Windows.h>
#endif

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
                received = 0;
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
            received = 0;
        }

        noDataForMs = 0;
        noDataDeadlineMs = baton->noDataTimeoutMs;
    }

    if(received > 0) {
        baton->push(buffer, received);
    } else {
        free(buffer);
    }

    if(baton->verbose) {
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
    int ct = 0;

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

        ct++;
        sendData(buffer, length);
    }

    // delay return until all data has been sent to js (prevents returing the promise before all data is sent)
    {
        std::unique_lock<std::mutex> lck(muSentData);
        while(this->sentCount < ct) sentDataSignal.wait(lck);
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

struct BufferDataJsInfo {
    char* buffer;
    int length;
    BufferedReadBaton* baton;
};

NAN_INLINE void sendBufferedReadBatonDataToJs(uv_work_t* req) {
  Nan::HandleScope scope;
  BufferDataJsInfo* data = static_cast<BufferDataJsInfo*>(req->data);

  v8::Local<v8::Value> argv[1];
  auto obj = Nan::NewBuffer(data->buffer, data->length); 
  argv[0] = obj.ToLocalChecked();
  
  Nan::AsyncResource resource("sendBufferedReadBatonDataToJs");
  v8::Local<v8::Function> callback_ = Nan::New(data->baton->onDataCallback);
  v8::Local<v8::Object> target = Nan::New<v8::Object>();
  
  resource.runInAsyncScope(target, callback_, 1, argv);

  {
    std::unique_lock<std::mutex> lck(data->baton->muSentData);
    data->baton->sentCount++;
    data->baton->sentDataSignal.notify_one();
  }
  
  delete req->data;
}

NAN_INLINE void noop_execute (uv_work_t* req) {
	// filler fx
}

void BufferedReadBaton::sendData(char* buffer, int length) {
    if(verbose) {
        muLogger.lock();
        auto out = defaultLogger();
        auto hex = bufferToHex(buffer, length);
        out << "BufferedReadBaton: send callback with data: " << hex << "\n";
        out.close();
        muLogger.unlock();
    }

    auto data = new BufferDataJsInfo;
    data->buffer = buffer;
    data->length = length;
    data->baton = this;

    uv_work_t *work = new uv_work_t;
    work->data = (void*)data;

    uv_queue_work(uv_default_loop(), work, noop_execute, (uv_after_work_cb)sendBufferedReadBatonDataToJs);
}

NAN_METHOD(BufferedRead)
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
