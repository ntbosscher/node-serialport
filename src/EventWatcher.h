
#include "./Util.h"
#include <nan.h>
#include <v8.h>
#include <thread>
#include <string>

#ifndef EventListener_h
#define EventListener_h

struct DeviceWatcher {
  bool verbose;
  HANDLE file;
  std::thread thread;
  Nan::Persistent<v8::Function> eventsCallback;
};

struct PortInfo {
  HANDLE fd;
  std::string path;
  std::thread* watcher;
};

struct LookupActivePortResult {
  HANDLE fd;
  std::string path;
};

std::thread EventWatcher(std::shared_ptr<DeviceWatcher> baton);

void markPortAsClosed(HANDLE file);
void markPortAsOpen(HANDLE file, char *path, std::thread thread);
bool portIsActive(DeviceWatcher *baton);
LookupActivePortResult* lookupActivePortByPath(std::string path);

#endif