#include "./EventWatcher.h"
#include <mutex>
#include <map>

std::mutex muActivePorts;
std::map<HANDLE,std::thread*> activePorts;

void markPortAsOpen(HANDLE file, std::thread thread) {
  muActivePorts.lock();
  activePorts[file] = &thread;
  muActivePorts.unlock();
}

void markPortAsClosed(HANDLE file) {
  muActivePorts.lock();
  
  auto pair = activePorts.find(file);
  auto threadIsValid = pair != activePorts.end();

  activePorts.erase(file);
  muActivePorts.unlock();

  try {
    if(threadIsValid && pair->second->joinable()) {
        pair->second->join();
    }
  } catch (...) {
    // sometimes on windows we get a NO_SUCH_PROCESS error here
    // ignore and carry on
  }
}

bool portIsActive(DeviceWatcher *baton) {
  
  muActivePorts.lock();
  auto active = activePorts.find(baton->file) != activePorts.end();
  muActivePorts.unlock();

  return active;
}