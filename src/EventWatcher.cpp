#include "./EventWatcher.h"
#include <mutex>
#include <map>
#include <string>
#include <memory>

std::mutex muActivePorts;
std::map<HANDLE,std::unique_ptr<PortInfo>> activePorts;

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

  for (auto it=activePorts.begin(); it!=activePorts.end(); ++it) {
    
    if(enableVerbose) {
      out << currentMs() << " lookupActivePortByPath: has " << (int)(size_t)it->second->fd << " " << (it->second->path) << "\n";
      out.flush();
    }

    if(it->second->path == path) {
      result = new LookupActivePortResult;
      result->fd = it->second->fd;
      result->path = it->second->path;

      if(enableVerbose) {
        out << currentMs() << " lookupActivePortByPath: match found: " << (int)(size_t)it->second->fd << " " << it->second->path << "\n";
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

void markPortAsOpen(HANDLE file, char *path, std::unique_ptr<std::thread> thread) {

  if(verboseLoggingEnabled()) {
      muLogger.lock();
      auto out = defaultLogger();
      out << currentMs() << " markPortAsOpen " << std::to_string((int)(size_t)file) + ": " << path << "\n";
      out.close();
      muLogger.unlock();
  }

  muActivePorts.lock();
  auto pi = std::make_unique<PortInfo>();
  
  pi->fd = file;
  pi->path = std::string(path);
  pi->watcher = std::move(thread);

  activePorts[file] = std::move(pi);
  muActivePorts.unlock();
}

void markPortAsClosed(HANDLE file) {
  auto verbose = verboseLoggingEnabled();
  if(verbose) {
      muLogger.lock();
      auto out = defaultLogger();
      out << currentMs() << " markPortAsClosed(" << (int)(size_t)file << ")\n";
      out.close();
      muLogger.unlock();
  }

  std::unique_ptr<PortInfo> info;

  muActivePorts.lock();

  auto pair = activePorts.find(file);

  if(pair != activePorts.end()) {
    info = std::move(pair->second);
    activePorts.erase(pair);
  }

  muActivePorts.unlock();

  auto threadIsValid = !!info;

  if(verbose) {
    muLogger.lock();
    auto out = defaultLogger();
    out << currentMs() << " markPortAsClosed: threadIsValid: " << threadIsValid << " " << (info && info->watcher->joinable()) << "\n";
    out.close();
    muLogger.unlock();
  }

  try {
    if(info && info->watcher->joinable()) {
        if(verbose) {
          muLogger.lock();
          auto out = defaultLogger();
          out << currentMs() << " markPortAsClosed: join thread\n";
          out.close();
          muLogger.unlock();
        }

        info->watcher->join();
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