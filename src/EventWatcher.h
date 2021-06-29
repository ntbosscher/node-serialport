
#include "./Util.h"
#include <nan.h>
#include <v8.h>
#include <thread>

#ifndef EventListener_h
#define EventListener_h

struct DeviceWatcher {
  bool verbose;
  HANDLE file;
  std::thread thread;
  Nan::Persistent<v8::Function> eventsCallback;
};

std::thread EventWatcher(DeviceWatcher *baton);

void markPortAsClosed(HANDLE file);
void markPortAsOpen(HANDLE file, std::thread thread);
bool portIsActive(DeviceWatcher *baton);

#endif