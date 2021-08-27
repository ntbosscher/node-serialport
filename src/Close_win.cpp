#include "./VoidBaton.h"
#include "./EventWatcher.h"
#include "./Util.h"
#include "./Close.h"

#ifdef WIN32
#define strncasecmp strnicmp
  #include "./win.h"
#endif

void EIO_Close(uv_work_t* req) {
    VoidBaton* data = static_cast<VoidBaton*>(req->data);
    
    if(verboseLoggingEnabled()) {
      muLogger.lock();
      auto out = defaultLogger();
      out << currentMs() << " CloseBaton " << std::to_string(data->fd) + " closing file\n";
      out.close();
      muLogger.unlock();
    }

    if (!ClosePort(int2handle(data->fd))) {
        ErrorCodeToString("Closing connection (CloseHandle)", GetLastError(), data->errorString);
        return;
    }

    markPortAsClosed((HANDLE)data->fd);
}

bool ClosePort(HANDLE fd) {
  HMODULE hKernel32 = LoadLibrary("kernel32.dll");

  // Look up function address
  CancelIoExType pCancelIoEx = (CancelIoExType)GetProcAddress(hKernel32, "CancelIoEx");

  // Do something with it
  if (pCancelIoEx) {
      // Function exists so call it
      // Cancel all pending IO Requests for the current device
      pCancelIoEx(fd, NULL);
  }

  return CloseHandle(fd);
}