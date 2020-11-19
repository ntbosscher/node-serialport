#include "./serialport.h"
#include "./win.h"
#include "./util.h"
#include <nan.h>
#include <list>
#include <vector>
#include <string.h>
#include <windows.h>
#include <Setupapi.h>
#include <initguid.h>
#include <devpkey.h>
#include <ctime>
#include <devguid.h>
#include <fstream>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <mutex>
#pragma comment(lib, "setupapi.lib")

std::list<int> g_closingHandles;

void ErrorCodeToString(const char* prefix, int errorCode, char *errorStr) {

    auto log = logger("error-code");
    log << errorCode << "\n";
    log.close();

  switch (errorCode) {
  case ERROR_FILE_NOT_FOUND:
    _snprintf_s(errorStr, ERROR_STRING_SIZE, _TRUNCATE, "%s: File not found", prefix);
    break;
  case ERROR_INVALID_HANDLE:
    _snprintf_s(errorStr, ERROR_STRING_SIZE, _TRUNCATE, "%s: Invalid handle", prefix);
    break;
  case ERROR_ACCESS_DENIED:
    _snprintf_s(errorStr, ERROR_STRING_SIZE, _TRUNCATE, "%s: Access denied", prefix);
    break;
  case ERROR_OPERATION_ABORTED:
    _snprintf_s(errorStr, ERROR_STRING_SIZE, _TRUNCATE, "%s: Operation aborted", prefix);
    break;
  case ERROR_INVALID_PARAMETER:
    _snprintf_s(errorStr, ERROR_STRING_SIZE, _TRUNCATE, "%s: The parameter is incorrect", prefix);
    break;
  default:
    _snprintf_s(errorStr, ERROR_STRING_SIZE, _TRUNCATE, "%s: Unknown error code %d", prefix, errorCode);
    break;
  }
}

void AsyncCloseCallback(uv_handle_t* handle) {
  uv_async_t* async = reinterpret_cast<uv_async_t*>(handle);
  delete async;
}

void EIO_Get(uv_work_t* req) {
  GetBaton* data = static_cast<GetBaton*>(req->data);

  DWORD bits = 0;
  if (!GetCommModemStatus(int2handle(data->fd), &bits)) {
    ErrorCodeToString("Getting control settings on COM port (GetCommModemStatus)", GetLastError(), data->errorString);
    return;
  }

  data->cts = bits & MS_CTS_ON;
  data->dsr = bits & MS_DSR_ON;
  data->dcd = bits & MS_RLSD_ON;
}

void EIO_GetBaudRate(uv_work_t* req) {
  GetBaudRateBaton* data = static_cast<GetBaudRateBaton*>(req->data);

  DCB dcb = { 0 };
  SecureZeroMemory(&dcb, sizeof(DCB));
  dcb.DCBlength = sizeof(DCB);

  if (!GetCommState(int2handle(data->fd), &dcb)) {
    ErrorCodeToString("Getting baud rate (GetCommState)", GetLastError(), data->errorString);
    return;
  }

  data->baudRate = static_cast<int>(dcb.BaudRate);
}

bool IsClosingHandle(int fd) {
  for (std::list<int>::iterator it = g_closingHandles.begin(); it != g_closingHandles.end(); ++it) {
    if (fd == *it) {
      g_closingHandles.remove(fd);
      return true;
    }
  }
  return false;
}

class Worker {
    std::mutex mutex;
    std::condition_variable condition;
    uv_async_t* data = nullptr;

public:
    uv_async_t* getNextJob() {
        std::unique_lock<std::mutex> lck(mutex);

        condition.wait(lck, [&]() { return data != nullptr; });

        auto async = data;
        data = nullptr;

        lck.unlock();
        condition.notify_all();

        return async;
    }

    void addJob(uv_async_t* async) {
        std::unique_lock<std::mutex> lck(mutex);
        condition.wait(lck, [&]() { return data == nullptr; });

        data = async;
        condition.notify_all();
    }
};

void EIO_Close(uv_work_t* req) {
  VoidBaton* data = static_cast<VoidBaton*>(req->data);

  g_closingHandles.push_back(data->fd);

  HMODULE hKernel32 = LoadLibrary("kernel32.dll");
  // Look up function address
  CancelIoExType pCancelIoEx = (CancelIoExType)GetProcAddress(hKernel32, "CancelIoEx");
  // Do something with it
  if (pCancelIoEx) {
    // Function exists so call it
    // Cancel all pending IO Requests for the current device
    pCancelIoEx(int2handle(data->fd), NULL);
  }
  if (!CloseHandle(int2handle(data->fd))) {
    ErrorCodeToString("Closing connection (CloseHandle)", GetLastError(), data->errorString);
    return;
  }
}



void EIO_Flush(uv_work_t* req) {
  VoidBaton* data = static_cast<VoidBaton*>(req->data);

  DWORD purge_all = PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR;
  if (!PurgeComm(int2handle(data->fd), purge_all)) {
    ErrorCodeToString("Flushing connection (PurgeComm)", GetLastError(), data->errorString);
    return;
  }
}

void EIO_Drain(uv_work_t* req) {
  VoidBaton* data = static_cast<VoidBaton*>(req->data);

  if (!FlushFileBuffers(int2handle(data->fd))) {
    ErrorCodeToString("Draining connection (FlushFileBuffers)", GetLastError(), data->errorString);
    return;
  }
}
