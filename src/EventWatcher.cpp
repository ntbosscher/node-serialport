#include "./EventWatcher.h"
#include <mutex>
#include <map>
#include <string>

std::mutex muActivePorts;
std::map<HANDLE,PortInfo*> activePorts;

LookupActivePortResult* lookupActivePortByPath(std::string path) {
  LookupActivePortResult* result = nullptr;

  auto enableVerbose = verboseLoggingEnabled();
  std::ofstream out;

  if(enableVerbose) {
      muLogger.lock();
      out = defaultLogger();
      out << currentMs() << " lookupActivePortByPath(" << path << ")\n";
      out.flush();
  }
  
  muActivePorts.lock();

  for (std::map<HANDLE,PortInfo*>::iterator it=activePorts.begin(); it!=activePorts.end(); ++it) {
    
    if(enableVerbose) {
      out << currentMs() << " lookupActivePortByPath: has " << (int)it->second->fd << " " << (it->second->path) << "\n";
      out.flush();
    }

    if(it->second->path == path) {
      result = new LookupActivePortResult;
      result->fd = it->second->fd;
      result->path = it->second->path;

      if(enableVerbose) {
        out << currentMs() << " lookupActivePortByPath: match found: " << (int)it->second->fd << " " << it->second->path << "\n";
        out.flush();
      }

      break;
    }
  }

  muActivePorts.unlock();

  if(enableVerbose) {
    out.close();
    muLogger.unlock();
  }

  return result;
}

void markPortAsOpen(HANDLE file, char *path, std::thread thread) {

  if(verboseLoggingEnabled()) {
      muLogger.lock();
      auto out = defaultLogger();
      out << currentMs() << " markPortAsOpen " << std::to_string((int)file) + ": " << path << "\n";
      out.close();
      muLogger.unlock();
  }

  muActivePorts.lock();
  auto pi = new PortInfo;
  
  pi->fd = file;
  pi->path = std::string(path);
  pi->watcher = &thread;

  activePorts[file] = pi;
  muActivePorts.unlock();
}

void markPortAsClosed(HANDLE file) {
  auto verbose = verboseLoggingEnabled();
  if(verbose) {
      muLogger.lock();
      auto out = defaultLogger();
      out << currentMs() << " markPortAsClosed(" << (int)file << ")\n";
      out.close();
      muLogger.unlock();
  }

  muActivePorts.lock();
  
  auto pair = activePorts.find(file);
  auto threadIsValid = pair != activePorts.end();

  if(threadIsValid) {
    activePorts.erase(file);
  }

  muActivePorts.unlock();

  if(verbose) {
    muLogger.lock();
    auto out = defaultLogger();
    out << currentMs() << " markPortAsClosed: threadIsValid: " << threadIsValid << " " << pair->second->watcher->joinable() << "\n";
    out.close();
    muLogger.unlock();
  }

  try {
    if(threadIsValid && pair->second->watcher->joinable()) {
        if(verbose) {
          muLogger.lock();
          auto out = defaultLogger();
          out << currentMs() << " markPortAsClosed: join thread\n";
          out.close();
          muLogger.unlock();
        }

        pair->second->watcher->join();
    }
  } catch (...) {
    // sometimes on windows we get a NO_SUCH_PROCESS error here
    // ignore and carry on
  }

  // cleanup the memory
  if(threadIsValid) {
    delete pair->second;
  }
}

bool portIsActive(DeviceWatcher *baton) {
  
  muActivePorts.lock();
  auto active = activePorts.find(baton->file) != activePorts.end();
  muActivePorts.unlock();

  return active;
}