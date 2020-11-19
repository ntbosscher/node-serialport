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

#endif  // PACKAGES_SERIALPORT_SRC_SERIALPORT_WIN_H_
