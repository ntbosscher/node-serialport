#include "./VoidBaton.h"
#include "./EventWatcher.h"
#include "./Util.h"

#ifdef WIN32
#define strncasecmp strnicmp
  #include "./win.h"
#endif

std::list<int> g_closingHandles;

bool IsClosingHandle(int fd) {
  for (std::list<int>::iterator it = g_closingHandles.begin(); it != g_closingHandles.end(); ++it) {
    if (fd == *it) {
      g_closingHandles.remove(fd);
      return true;
    }
  }
  return false;
}

void EIO_Close(uv_work_t* req) {
    VoidBaton* data = static_cast<VoidBaton*>(req->data);
    g_closingHandles.push_back(data->fd);

    markPortAsClosed((HANDLE)data->fd);

    HMODULE hKernel32 = LoadLibrary("kernel32.dll");
    // Look up function address
    CancelIoExType pCancelIoEx = (CancelIoExType)GetProcAddress(hKernel32, "CancelIoEx");
    // Do something with it
    if (pCancelIoEx) {
        // Function exists so call it
        // Cancel all pending IO Requests for the current device
        pCancelIoEx(int2handle(data->fd), NULL);
    }

    if(verboseLoggingEnabled()) {
      auto out = defaultLogger();
      out << currentMs() << " CloseBaton " << std::to_string(data->fd) + " closing file\n";
      out.close();
    }

    if (!CloseHandle(int2handle(data->fd))) {
        ErrorCodeToString("Closing connection (CloseHandle)", GetLastError(), data->errorString);
        return;
    }
}
