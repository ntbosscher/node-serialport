#include <nan.h>
#include <v8.h>
#include "./Win.h"
#include "./Util.h"
#include "./EventWatcher.h"
#include <thread>

NAN_INLINE void noop_execute (uv_work_t* req) {
	// filler fx
}

struct EventWatcherInfo {
  char Error[ERROR_STRING_SIZE];
  int ErrorCode;
  int Event;
  DeviceWatcher *baton;
};

NAN_INLINE void EventWatcherJsCallback(uv_work_t* req) {
  Nan::HandleScope scope;
  EventWatcherInfo* data = static_cast<EventWatcherInfo*>(req->data);

  v8::Local<v8::Object> target = Nan::New<v8::Object>();
  v8::Local<v8::Value> argv[2];

  if(data->Error[0]) {
      argv[0] = v8::Exception::Error(Nan::New<v8::String>(data->Error).ToLocalChecked());
      
      auto obj = Nan::New<v8::Object>();
      v8::Local<v8::Context> context = Nan::GetCurrentContext();
      obj->Set(context, Nan::New("errorCode").ToLocalChecked(), Nan::New(data->ErrorCode));
      argv[1] = obj;
  } else {
      argv[0] = Nan::Null();
      
      auto obj = Nan::New<v8::Object>();
      v8::Local<v8::Context> context = Nan::GetCurrentContext();
      obj->Set(context, Nan::New("event").ToLocalChecked(), Nan::New(data->Event));
      argv[1] = obj;
  }

  Nan::AsyncResource resource("EventWatcherJsCallback");
  v8::Local<v8::Function> callback_ = Nan::New(data->baton->eventsCallback);
  resource.runInAsyncScope(target, callback_, 2, argv);
  
  delete req->data;
}

void _eventWatcherEmit(EventWatcherInfo *result) {
    
    uv_work_t *work = new uv_work_t;
    work->data = (void*)result;

    uv_queue_work(uv_default_loop(), work, noop_execute, (uv_after_work_cb)EventWatcherJsCallback);
}

int _waitForCommOV(HANDLE file, OVERLAPPED *ov, bool verbose) {
  for(;;) {
    switch(WaitForSingleObject(ov->hEvent, 50)) {
      case WAIT_ABANDONED:
        return -1;
      case WAIT_TIMEOUT:
        if(verbose) {
          auto out = defaultLogger();
          out << currentMs() << " _eventWatcher: wait timeout\n";
          out.close();
        }
        
        return 0;
      case WAIT_FAILED:
        char error[ERROR_STRING_SIZE];
        ErrorCodeToString("WaitForSingleObject", GetLastError(), error);

        if(verbose) {
          auto out = defaultLogger();
          out << currentMs() << " _eventWatcher: WaitForSingleObject received error " << error << "\n";
          out.close();
        }

        return -1;
      case WAIT_OBJECT_0:
        DWORD garbage;

        if(!GetOverlappedResult(file, ov, &garbage, true)) {
            char error[ERROR_STRING_SIZE];
            ErrorCodeToString("GetOverlappedResult", GetLastError(), error);

            if(verbose) {
              auto out = defaultLogger();
              out << currentMs() << " _eventWatcher: GetOverlappedResult received error " << error << "\n";
              out.close();
            }

            return -1;
        }

        return 1;
    }
  }    
}

EventWatcherInfo* _newEventWatcher(DeviceWatcher *baton) {
  auto result = new EventWatcherInfo;
  result->baton = baton;
  result->Error[0] = '\0';
  result->ErrorCode = 0;

  return result;
}

void _eventWatcher(DeviceWatcher *baton) {

  auto file = baton->file;
  auto verbose = baton->verbose;

  if(verbose) {
    auto out = defaultLogger();
    out << currentMs() << " _eventWatcher: thread started (handle=" << file << ")\n";
    out.close();
  }

  auto deadline = currentMs() + 5000;

  auto mask = EV_CTS | EV_DSR | EV_BREAK | EV_RING | EV_RLSD;

  if(!SetCommMask(file, mask)) {
    
    EventWatcherInfo *result = new EventWatcherInfo;
    result->baton = baton;
    result->Error[0] = '\0';

    auto error = GetLastError();
    ErrorCodeToString("SetCommMask", error, result->Error);
    result->ErrorCode = error;

    if(verbose) {
      auto out = defaultLogger();
      out << currentMs() << " _eventWatcher: SetCommMask received error " << result->Error << "\n";
      out.close();
    }

    _eventWatcherEmit(result);
    return;
  }

  DWORD value = 0;
  OVERLAPPED ov;
  bool pending = false;

  while(portIsActive(baton)) {
    if(!pending) {
      memset(&ov, 0, sizeof(ov));
      ov.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);

      if(!WaitCommEvent(file, &value, &ov)) {
        auto error = GetLastError();
        if(error != ERROR_IO_PENDING) {

          if(!portIsActive(baton)) {
            return;
          }

          auto result = _newEventWatcher(baton);
          ErrorCodeToString("WaitCommEvent", error, result->Error);
          result->ErrorCode = error;

          if(verbose) {
            auto out = defaultLogger();
            out << currentMs() << " _eventWatcher: WaitCommEvent received error " << result->Error << "\n";
            out.close();
          }

          _eventWatcherEmit(result);

          if(error == ERROR_INVALID_HANDLE) {
            return;
          }

          continue;
        }

        pending = true;
      }
    }

    if(pending) {
      int result = _waitForCommOV(file, &ov, verbose);
      if(result == 0) {
        continue;
      }

      pending = false;
    }
    
    auto result = _newEventWatcher(baton);
    result->Event = value;
    _eventWatcherEmit(result);
  }

  if(verbose) {
    auto out = defaultLogger();
    out << currentMs() << " _eventWatcher: exited\n";
    out.close();
  }
}

std::thread EventWatcher(DeviceWatcher *baton) {
  std::thread t(_eventWatcher, baton);
  return t;
}