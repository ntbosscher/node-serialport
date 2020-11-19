#ifndef PACKAGES_SERIALPORT_SRC_SERIALPORT_WIN_H_
#define PACKAGES_SERIALPORT_SRC_SERIALPORT_WIN_H_

#include <nan.h>
#include <list>
#include <v8.h>
#include <string>

#define MAX_BUFFER_SIZE 1000

// As per https://msdn.microsoft.com/en-us/library/windows/desktop/ms724872(v=vs.85).aspx
#define MAX_REGISTRY_KEY_SIZE 255

// Declare type of pointer to CancelIoEx function
typedef BOOL (WINAPI *CancelIoExType)(HANDLE hFile, LPOVERLAPPED lpOverlapped);

static inline HANDLE int2handle(int ptr) {
  return reinterpret_cast<HANDLE>(static_cast<uintptr_t>(ptr));
}

struct ReadBaton : public Nan::AsyncResource {
  ReadBaton() : AsyncResource("node-serialport:ReadBaton"), errorString() {}
  int fd = 0;
  int timeout = -1;
  char* bufferData = nullptr;
  size_t bufferLength = 0;
  size_t bytesRead = 0;
  size_t bytesToRead = 0;
  size_t offset = 0;
  void* hThread = nullptr;
  bool complete = false;
  char errorString[ERROR_STRING_SIZE];
  Nan::Callback callback;
};

NAN_METHOD(Read);
void EIO_Read(uv_work_t* req);
void EIO_AfterRead(uv_async_t* req);
DWORD __stdcall ReadThread(LPVOID param);


NAN_METHOD(List);
void EIO_List(uv_work_t* req);
void EIO_AfterList(uv_work_t* req);

struct ListResultItem {
  std::string path;
  std::string manufacturer;
  std::string serialNumber;
  std::string pnpId;
  std::string locationId;
  std::string vendorId;
  std::string productId;
};

void internalInit();

#endif  // PACKAGES_SERIALPORT_SRC_SERIALPORT_WIN_H_
