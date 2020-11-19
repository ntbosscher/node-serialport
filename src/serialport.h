#ifndef PACKAGES_SERIALPORT_SRC_SERIALPORT_H_
#define PACKAGES_SERIALPORT_SRC_SERIALPORT_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nan.h>
#include <string>
#include "./OpenBaton.h"
#include "./util.h"

#define ERROR_STRING_SIZE 1024

NAN_METHOD(Close);
void EIO_Close(uv_work_t* req);
void EIO_AfterClose(uv_work_t* req);

NAN_METHOD(Flush);
void EIO_Flush(uv_work_t* req);
void EIO_AfterFlush(uv_work_t* req);

NAN_METHOD(Set);
void EIO_Set(uv_work_t* req);
void EIO_AfterSet(uv_work_t* req);

NAN_METHOD(Get);
void EIO_Get(uv_work_t* req);
void EIO_AfterGet(uv_work_t* req);

NAN_METHOD(GetBaudRate);
void EIO_GetBaudRate(uv_work_t* req);
void EIO_AfterGetBaudRate(uv_work_t* req);

NAN_METHOD(Drain);
void EIO_Drain(uv_work_t* req);
void EIO_AfterDrain(uv_work_t* req);

struct SetBaton : public Nan::AsyncResource {
  SetBaton() : AsyncResource("node-serialport:SetBaton"), errorString() {}
  int fd = 0;
  Nan::Callback callback;
  int result = 0;
  char errorString[ERROR_STRING_SIZE];
  bool rts = false;
  bool cts = false;
  bool dtr = false;
  bool dsr = false;
  bool brk = false;
};

struct GetBaton : public Nan::AsyncResource {
  GetBaton() : AsyncResource("node-serialport:GetBaton"), errorString() {}
  int fd = 0;
  Nan::Callback callback;
  char errorString[ERROR_STRING_SIZE];
  bool cts = false;
  bool dsr = false;
  bool dcd = false;
};

struct GetBaudRateBaton : public Nan::AsyncResource {
  GetBaudRateBaton() :
    AsyncResource("node-serialport:GetBaudRateBaton"), errorString() {}
  int fd = 0;
  Nan::Callback callback;
  char errorString[ERROR_STRING_SIZE];
  int baudRate = 0;
};

struct VoidBaton : public Nan::AsyncResource {
  VoidBaton() : AsyncResource("node-serialport:VoidBaton"), errorString() {}
  int fd = 0;
  Nan::Callback callback;
  char errorString[ERROR_STRING_SIZE];
};

int setup(int fd, OpenBaton *data);
#endif  // PACKAGES_SERIALPORT_SRC_SERIALPORT_H_
