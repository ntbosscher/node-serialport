
#include "./BufferedReadBaton.h"
#include "./V8ArgDecoder.h"
#include <algorithm>
#include "./ReadBaton.h"

#ifdef WIN32
#include <Windows.h>
#endif

#include <chrono>
#include <thread>

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

    // baton->logVerbose("started reader thread");

    for(;;) {
        
        if(noDataForMs > noDataDeadlineMs) {
            // baton->logVerbose("timed out after 10s");
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

            // baton->logVerbose(std::string("got error: ") + std::string(errorString));
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

    // baton->logVerbose(std::string("finished, got ") + std::to_string(totalCount) + std::string(" bytes\n"));

    {
        std::unique_lock<std::mutex> lck(baton->syncMutex);
        baton->readThreadIsRunning = false;
        baton->signal.notify_one();
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

NAN_INLINE void noop_execute (uv_work_t* req) {
	// filler fx
}

class BufferedReadResponseBaton: public BatonBase {
public:
    BufferedReadBaton *parent;
    char* buffer;
    int length;
    bool found = false;
    int id = 0;

    BufferedReadResponseBaton(BufferedReadBaton *parent, std::string name, v8::Local<v8::Function> callback_, int id) : BatonBase(name, callback_)
    {
        request.data = this;
        this->parent = parent;
        this->id = id;
        this->isSingleResult = true;
    }

    v8::Local<v8::Function> getCallback() override { 
        if(found) {
           // setup next chunk responder
           // do this here to ensure we're in the active v8 scope
           BufferedReadResponseBaton* res = new BufferedReadResponseBaton(parent, debugName, Nan::New(callback), this->id+1);
           res->start();
        }

        v8::Local<v8::Function> cb;

        if(found) {
            cb = Nan::New(parent->onData);
        } else {
            cb = Nan::New(parent->onDone);
        }

        return cb;
    }

    v8::Local<v8::Value> getReturnValue() override {
        if(found) {
            return Nan::NewBuffer(buffer, length).ToLocalChecked();
        } else {
            return Nan::False();
        }
    }

    void run() override {
        int ct = 0;
        logVerbose("run");

        {
            std::unique_lock<std::mutex> lck(parent->syncMutex);

            while(parent->queue.empty() && parent->readThreadIsRunning) parent->signal.wait(lck);
            if(parent->queue.empty()) {
                logVerbose("queue-empty");
                return;
            }
            
            auto next = parent->queue.front();
            buffer = next->buffer;
            length = next->length;
            found = true;

            parent->queue.pop();
        }

        logVerbose("done");
    }
};

NAN_METHOD(BufferedRead)
{
    V8ArgDecoder args(&info);

    auto fd = args.takeInt32();
    auto noDataTimeoutMs = args.takeInt32();
    auto cb = args.takeFunction();
    auto done = args.takeFunction();

    if(args.hasError()) return;

    BufferedReadBaton *baton = new BufferedReadBaton();

    baton->fd = fd;
    baton->noDataTimeoutMs = noDataTimeoutMs;
    baton->onData.Reset(cb);
    baton->onDone.Reset(done);
    baton->readThreadIsRunning = true;

    auto response = new BufferedReadResponseBaton(baton, "buffered-read-response-baton", cb, 1);

    std::thread t1(reader, baton);
    t1.detach();

    response->start();
}
