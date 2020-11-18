#include "./serialport.h"
#include "./serialport_win.h"
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

#define ARRAY_SIZE(arr)     (sizeof(arr)/sizeof(arr[0]))

#define MAX_BUFFER_SIZE 1000

// As per https://msdn.microsoft.com/en-us/library/windows/desktop/ms724872(v=vs.85).aspx
#define MAX_REGISTRY_KEY_SIZE 255

#define TIMEOUT_PRECISION 10

// Declare type of pointer to CancelIoEx function
typedef BOOL (WINAPI *CancelIoExType)(HANDLE hFile, LPOVERLAPPED lpOverlapped);

static inline HANDLE int2handle(int ptr) {
  return reinterpret_cast<HANDLE>(static_cast<uintptr_t>(ptr));
}

std::list<int> g_closingHandles;

std::ofstream logger(std::string id) {
    std::string fileName = "C:\\Users\\Nate\\AppData\\Local\\console.log." + id + ".txt";
    return std::ofstream(fileName, std::ofstream::app);
}

void send_async(uv_async_t* handle) {
    auto log = logger("async-errors");
    log << "called" << "\n";
    log.flush();

    int err = uv_async_send(handle);
    if (err == 0) return;
    
    
    log << uv_strerror(err) << "\n";
    log.close();
}

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

void EIO_Open(uv_work_t* req) {
  OpenBaton* data = static_cast<OpenBaton*>(req->data);

  char originalPath[1024];
  strncpy_s(originalPath, sizeof(originalPath), data->path, _TRUNCATE);
  // data->path is char[1024] but on Windows it has the form "COMx\0" or "COMxx\0"
  // We want to prepend "\\\\.\\" to it before we call CreateFile
  strncpy(data->path + 20, data->path, 10);
  strncpy(data->path, "\\\\.\\", 4);
  strncpy(data->path + 4, data->path + 20, 10);

  int shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
  if (data->lock) {
    shareMode = 0;
  }

  HANDLE file = CreateFile(
    data->path,
    GENERIC_READ | GENERIC_WRITE,
    shareMode,  // dwShareMode 0 Prevents other processes from opening if they request delete, read, or write access
    NULL,
    OPEN_EXISTING,
    FILE_FLAG_OVERLAPPED,  // allows for reading and writing at the same time and sets the handle for asynchronous I/O
    NULL);

  if (file == INVALID_HANDLE_VALUE) {
    DWORD errorCode = GetLastError();
    char temp[100];
    _snprintf_s(temp, sizeof(temp), _TRUNCATE, "Opening %s", originalPath);
    ErrorCodeToString(temp, errorCode, data->errorString);
    return;
  }

  DCB dcb = { 0 };
  SecureZeroMemory(&dcb, sizeof(DCB));
  dcb.DCBlength = sizeof(DCB);

  if (!GetCommState(file, &dcb)) {
    ErrorCodeToString("Open (GetCommState)", GetLastError(), data->errorString);
    CloseHandle(file);
    return;
  }

  if (data->hupcl) {
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
  } else {
    dcb.fDtrControl = DTR_CONTROL_DISABLE;  // disable DTR to avoid reset
  }

  dcb.Parity = NOPARITY;
  dcb.ByteSize = 8;
  dcb.StopBits = ONESTOPBIT;


  dcb.fOutxDsrFlow = FALSE;
  dcb.fOutxCtsFlow = FALSE;

  if (data->xon) {
    dcb.fOutX = TRUE;
  } else {
    dcb.fOutX = FALSE;
  }

  if (data->xoff) {
    dcb.fInX = TRUE;
  } else {
    dcb.fInX = FALSE;
  }

  if (data->rtscts) {
    dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
    dcb.fOutxCtsFlow = TRUE;
  } else {
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
  }

  dcb.fBinary = true;
  dcb.BaudRate = data->baudRate;
  dcb.ByteSize = data->dataBits;

  switch (data->parity) {
  case SERIALPORT_PARITY_NONE:
    dcb.Parity = NOPARITY;
    break;
  case SERIALPORT_PARITY_MARK:
    dcb.Parity = MARKPARITY;
    break;
  case SERIALPORT_PARITY_EVEN:
    dcb.Parity = EVENPARITY;
    break;
  case SERIALPORT_PARITY_ODD:
    dcb.Parity = ODDPARITY;
    break;
  case SERIALPORT_PARITY_SPACE:
    dcb.Parity = SPACEPARITY;
    break;
  }

  switch (data->stopBits) {
  case SERIALPORT_STOPBITS_ONE:
    dcb.StopBits = ONESTOPBIT;
    break;
  case SERIALPORT_STOPBITS_ONE_FIVE:
    dcb.StopBits = ONE5STOPBITS;
    break;
  case SERIALPORT_STOPBITS_TWO:
    dcb.StopBits = TWOSTOPBITS;
    break;
  }

  if (!SetCommState(file, &dcb)) {
    ErrorCodeToString("Open (SetCommState)", GetLastError(), data->errorString);
    CloseHandle(file);
    return;
  }

  // Set the timeouts for read and write operations.
  // Read operation will wait for at least 1 byte to be received.
  COMMTIMEOUTS commTimeouts = {};
  commTimeouts.ReadIntervalTimeout = 0;          // Never timeout, always wait for data.
  commTimeouts.ReadTotalTimeoutMultiplier = 0;   // Do not allow big read timeout when big read buffer used
  commTimeouts.ReadTotalTimeoutConstant = 0;     // Total read timeout (period of read loop)
  commTimeouts.WriteTotalTimeoutConstant = 0;    // Const part of write timeout
  commTimeouts.WriteTotalTimeoutMultiplier = 0;  // Variable part of write timeout (per byte)

  if (!SetCommTimeouts(file, &commTimeouts)) {
    ErrorCodeToString("Open (SetCommTimeouts)", GetLastError(), data->errorString);
    CloseHandle(file);
    return;
  }

  // Remove garbage data in RX/TX queues
  PurgeComm(file, PURGE_RXCLEAR);
  PurgeComm(file, PURGE_TXCLEAR);

  data->result = static_cast<int>(reinterpret_cast<uintptr_t>(file));
}

void EIO_Update(uv_work_t* req) {
  ConnectionOptionsBaton* data = static_cast<ConnectionOptionsBaton*>(req->data);

  DCB dcb = { 0 };
  SecureZeroMemory(&dcb, sizeof(DCB));
  dcb.DCBlength = sizeof(DCB);

  if (!GetCommState(int2handle(data->fd), &dcb)) {
    ErrorCodeToString("Update (GetCommState)", GetLastError(), data->errorString);
    return;
  }

  dcb.BaudRate = data->baudRate;

  if (!SetCommState(int2handle(data->fd), &dcb)) {
    ErrorCodeToString("Update (SetCommState)", GetLastError(), data->errorString);
    return;
  }
}

void EIO_Set(uv_work_t* req) {
  SetBaton* data = static_cast<SetBaton*>(req->data);

  if (data->rts) {
    EscapeCommFunction(int2handle(data->fd), SETRTS);
  } else {
    EscapeCommFunction(int2handle(data->fd), CLRRTS);
  }

  if (data->dtr) {
    EscapeCommFunction(int2handle(data->fd), SETDTR);
  } else {
    EscapeCommFunction(int2handle(data->fd), CLRDTR);
  }

  if (data->brk) {
    EscapeCommFunction(int2handle(data->fd), SETBREAK);
  } else {
    EscapeCommFunction(int2handle(data->fd), CLRBREAK);
  }

  DWORD bits = 0;

  GetCommMask(int2handle(data->fd), &bits);

  bits &= ~(EV_CTS | EV_DSR);

  if (data->cts) {
    bits |= EV_CTS;
  }

  if (data->dsr) {
    bits |= EV_DSR;
  }

  if (!SetCommMask(int2handle(data->fd), bits)) {
    ErrorCodeToString("Setting options on COM port (SetCommMask)", GetLastError(), data->errorString);
    return;
  }
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

void EIO_AfterWrite(uv_async_t* req) {
  Nan::HandleScope scope;
  WriteBaton* baton = static_cast<WriteBaton*>(req->data);
  uv_close(reinterpret_cast<uv_handle_t*>(req), AsyncCloseCallback);

  v8::Local<v8::Value> argv[1];
  if (baton->errorString[0]) {
    argv[0] = v8::Exception::Error(Nan::New<v8::String>(baton->errorString).ToLocalChecked());
  } else {
    argv[0] = Nan::Null();
  }
  baton->callback.Call(1, argv, baton);
  baton->buffer.Reset();
  delete baton;
}

int currentMs() {
  return clock() / (CLOCKS_PER_SEC / 1000);
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
///                                                                                                       ///
///                                                                                                       ///
///  Writer                                                                                               ///
///                                                                                                       ///
///                                                                                                       ///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Writer : public Worker {
    int writeHandle(WriteBaton* baton, bool blocking) {

        OVERLAPPED* ov = new OVERLAPPED;

        // Set the timeout to MAXDWORD in order to disable timeouts, so the read operation will
        // return immediately no matter how much data is available.
        COMMTIMEOUTS commTimeouts = {};

        if (blocking) {
            commTimeouts.ReadIntervalTimeout = TIMEOUT_PRECISION;
        }
        else {
            commTimeouts.ReadIntervalTimeout = MAXDWORD;
        }

        if (!SetCommTimeouts(int2handle(baton->fd), &commTimeouts)) {
            int lastError = GetLastError();
            ErrorCodeToString("Setting COM timeout (SetCommTimeouts)", lastError, baton->errorString);
            baton->complete = true;
            return 0;
        }

        // Store additional data after whatever data has already been read.
        char* offsetPtr = baton->bufferData + baton->offset;

        // ReadFile, unlike ReadFileEx, needs an event in the overlapped structure.
        memset(ov, 0, sizeof(OVERLAPPED));
        ov->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        DWORD bytesTransferred = 0;

        if (!WriteFile(int2handle(baton->fd), offsetPtr, baton->bufferLength - baton->offset, NULL, ov)) {
            int errorCode = GetLastError();
            if (errorCode != ERROR_IO_PENDING) {
                ErrorCodeToString("Writing to COM port (WriteFileEx)", errorCode, baton->errorString);
                baton->complete = true;
                return 0;
            }

            if (!GetOverlappedResult(int2handle(baton->fd), ov, &bytesTransferred, true)) {
                int errorCode = GetLastError();
                ErrorCodeToString("Writing to COM port (WriteFileEx)", errorCode, baton->errorString);
                baton->complete = true;
                return 0;
            }
        }

        CloseHandle(ov->hEvent);

        baton->offset += bytesTransferred;
        baton->complete = baton->offset == baton->bufferLength;
        return bytesTransferred;
    }

public:
    void thread() {
        auto out = logger("writer");

        while (true) {
            auto async = getNextJob();

            WriteBaton* baton = static_cast<WriteBaton*>(async->data);

            int start = currentMs();
            out << currentMs() << " WriteThread: start write\n";
            int deadline = start + baton->timeout;

            do {

                writeHandle(baton, true);

                if (baton->bytesWritten == baton->bufferLength) {
                    baton->complete = true;
                }
            } while(!baton->complete && currentMs() > deadline);

            out << currentMs() << " WriteThread: done, took " << (currentMs() - start) << "ms\n";
            out.flush();

            // Signal the main thread to run the callback.
            send_async(async);
        }
    }
};

Writer* writer; 

NAN_METHOD(Write) {

    // file descriptor
    if (!info[0]->IsInt32()) {
        Nan::ThrowTypeError("First argument must be an int");
        return;
    }

    int fd = Nan::To<int>(info[0]).FromJust();
    
    // buffer
    if (!info[1]->IsObject() || !node::Buffer::HasInstance(info[1])) {
        Nan::ThrowTypeError("Second argument must be a buffer");
        return;
    }

    v8::Local<v8::Object> buffer = Nan::To<v8::Object>(info[1]).ToLocalChecked();
    char* bufferData = node::Buffer::Data(buffer);
    size_t bufferLength = node::Buffer::Length(buffer);

    // timeout
    if (!info[2]->IsInt32()) {
        Nan::ThrowTypeError("Second argument must be an int");
        return;
    }

    int timeout = Nan::To<v8::Int32>(info[2]).ToLocalChecked()->Value();

    // callback
    if (!info[3]->IsFunction()) {
        Nan::ThrowTypeError("Third argument must be a function");
        return;
    }

    WriteBaton* baton = new WriteBaton();
    baton->fd = fd;
    baton->timeout = timeout;
    baton->buffer.Reset(buffer);
    baton->bufferData = bufferData;
    baton->bufferLength = bufferLength;
    baton->offset = 0;
    baton->callback.Reset(info[3].As<v8::Function>());
    baton->complete = false;

    uv_async_t* async = new uv_async_t;
    uv_async_init(uv_default_loop(), async, EIO_AfterWrite);
    async->data = baton;

    writer->addJob(async);
}

void __stdcall WriteIOCompletion(DWORD errorCode, DWORD bytesTransferred, OVERLAPPED* ov) {
    WriteBaton* baton = static_cast<WriteBaton*>(ov->hEvent);
    DWORD bytesWritten;
    if (!GetOverlappedResult(int2handle(baton->fd), ov, &bytesWritten, TRUE)) {
        errorCode = GetLastError();
        ErrorCodeToString("Writing to COM port (GetOverlappedResult)", errorCode, baton->errorString);
        baton->complete = true;
        return;
    }
    if (bytesWritten) {
        baton->offset += bytesWritten;
        if (baton->offset >= baton->bufferLength) {
            baton->complete = true;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
///                                                                                                       ///
///                                                                                                       ///
///  READER                                                                                               ///
///                                                                                                       ///
///                                                                                                       ///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Reader : public Worker {

    int readHandle(ReadBaton* baton, bool blocking) {
        OVERLAPPED* ov = new OVERLAPPED;

        // Set the timeout to MAXDWORD in order to disable timeouts, so the read operation will
        // return immediately no matter how much data is available.
        COMMTIMEOUTS commTimeouts = {};

        if (blocking) {
            commTimeouts.ReadIntervalTimeout = TIMEOUT_PRECISION;
        }
        else {
            commTimeouts.ReadIntervalTimeout = MAXDWORD;
        }

        if (!SetCommTimeouts(int2handle(baton->fd), &commTimeouts)) {
            int lastError = GetLastError();
            ErrorCodeToString("Setting COM timeout (SetCommTimeouts)", lastError, baton->errorString);
            baton->complete = true;
            return 0;
        }

        // Store additional data after whatever data has already been read.
        char* offsetPtr = baton->bufferData + baton->offset;

        // ReadFile, unlike ReadFileEx, needs an event in the overlapped structure.
        memset(ov, 0, sizeof(OVERLAPPED));
        ov->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        DWORD bytesTransferred = 0;

        if (!ReadFile(int2handle(baton->fd), offsetPtr, baton->bytesToRead, &bytesTransferred, ov)) {
            int errorCode = GetLastError();

            if (errorCode != ERROR_IO_PENDING) {
                ErrorCodeToString("Reading from COM port (ReadFile)", errorCode, baton->errorString);
                baton->complete = true;
                CloseHandle(ov->hEvent);
                return 0;
            }

            if (!GetOverlappedResult(int2handle(baton->fd), ov, &bytesTransferred, TRUE)) {
                int lastError = GetLastError();
                ErrorCodeToString("Reading from COM port (GetOverlappedResult)", lastError, baton->errorString);
                baton->complete = true;
                CloseHandle(ov->hEvent);
                return 0;
            }
        }

        CloseHandle(ov->hEvent);

        baton->bytesToRead -= bytesTransferred;
        baton->bytesRead += bytesTransferred;
        baton->offset += bytesTransferred;
        baton->complete = baton->bytesToRead == 0;
        return bytesTransferred;
    }

public:
    void thread() {

        auto out = logger("reader");

        while (true) {
            auto async = getNextJob();
            ReadBaton* baton = static_cast<ReadBaton*>(async->data);

            int start = currentMs();
            out << currentMs() << " ReadThreader: start read\n";

            int deadline = start + baton->timeout;

            do {

                readHandle(baton, true);

                if (baton->bytesToRead == 0) {
                    baton->complete = true;
                }

            } while (!baton->complete && currentMs() > deadline);

            out << currentMs() << " ReadThreader: done, took " << (currentMs() - start) << "ms\n";
            out.flush();

            // Signal the main thread to run the callback.
            send_async(async);
        }
    }
};


Reader* reader;

NAN_METHOD(Read) {
  // file descriptor
  if (!info[0]->IsInt32()) {
    Nan::ThrowTypeError("First argument must be a fd");
    return;
  }
  int fd = Nan::To<int>(info[0]).FromJust();

  // buffer
  if (!info[1]->IsObject() || !node::Buffer::HasInstance(info[1])) {
    Nan::ThrowTypeError("Second argument must be a buffer");
    return;
  }
  v8::Local<v8::Object> buffer = Nan::To<v8::Object>(info[1]).ToLocalChecked();
  size_t bufferLength = node::Buffer::Length(buffer);

  // offset
  if (!info[2]->IsInt32()) {
    Nan::ThrowTypeError("Third argument must be an int");
    return;
  }
  int offset = Nan::To<v8::Int32>(info[2]).ToLocalChecked()->Value();

  // bytes to read
  if (!info[3]->IsInt32()) {
    Nan::ThrowTypeError("Fourth argument must be an int");
    return;
  }

  size_t bytesToRead = Nan::To<v8::Int32>(info[3]).ToLocalChecked()->Value();

  if ((bytesToRead + offset) > bufferLength) {
    Nan::ThrowTypeError("'bytesToRead' + 'offset' cannot be larger than the buffer's length");
    return;
  }

  // timeout
  if (!info[4]->IsInt32()) {
      Nan::ThrowTypeError("Fifth argument must be an int");
      return;
  }

  int timeout = Nan::To<v8::Int32>(info[4]).ToLocalChecked()->Value();

  // callback
  if (!info[5]->IsFunction()) {
    Nan::ThrowTypeError("Sixth argument must be a function");
    return;
  }

  ReadBaton* baton = new ReadBaton();
  baton->fd = fd;
  baton->offset = offset;
  baton->bytesToRead = bytesToRead;
  baton->bufferLength = bufferLength;
  baton->bufferData = node::Buffer::Data(buffer);
  baton->timeout = timeout;
  baton->callback.Reset(info[5].As<v8::Function>());
  baton->complete = false;

  uv_async_t* async = new uv_async_t;
  uv_async_init(uv_default_loop(), async, EIO_AfterRead);
  async->data = baton;

  reader->addJob(async);
}

void EIO_AfterRead(uv_async_t* req) {
  Nan::HandleScope scope;
  ReadBaton* baton = static_cast<ReadBaton*>(req->data);
  uv_close(reinterpret_cast<uv_handle_t*>(req), AsyncCloseCallback);

  v8::Local<v8::Value> argv[2];
  if (baton->errorString[0]) {
    argv[0] = Nan::Error(baton->errorString);
    argv[1] = Nan::Undefined();
  } else {
    argv[0] = Nan::Null();
    argv[1] = Nan::New<v8::Integer>(static_cast<int>(baton->bytesRead));
  }

  baton->callback.Call(2, argv, baton);
  delete baton;
}

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

char *copySubstring(char *someString, int n) {
  char *new_ = reinterpret_cast<char*>(malloc(sizeof(char)*n + 1));
  strncpy_s(new_, n + 1, someString, n);
  new_[n] = '\0';
  return new_;
}

// It's possible that the s/n is a construct and not the s/n of the parent USB
// composite device. This performs some convoluted registry lookups to fetch the USB s/n.
void getSerialNumber(const char *vid,
                     const char *pid,
                     const HDEVINFO hDevInfo,
                     SP_DEVINFO_DATA deviceInfoData,
                     const unsigned int maxSerialNumberLength,
                     char* serialNumber) {
  _snprintf_s(serialNumber, maxSerialNumberLength, _TRUNCATE, "");
  if (vid == NULL || pid == NULL) {
    return;
  }

  DWORD dwSize;
  WCHAR szWUuidBuffer[MAX_BUFFER_SIZE];
  WCHAR containerUuid[MAX_BUFFER_SIZE];


  // Fetch the "Container ID" for this device node. In USB context, this "Container
  // ID" refers to the composite USB device, i.e. the USB device as a whole, not
  // just one of its interfaces with a serial port driver attached.

  // From https://stackoverflow.com/questions/3438366/setupdigetdeviceproperty-usage-example:
  // Because this is not compiled with UNICODE defined, the call to SetupDiGetDevicePropertyW
  // has to be setup manually.
  DEVPROPTYPE ulPropertyType;
  typedef BOOL (WINAPI *FN_SetupDiGetDevicePropertyW)(
    __in       HDEVINFO DeviceInfoSet,
    __in       PSP_DEVINFO_DATA DeviceInfoData,
    __in       const DEVPROPKEY *PropertyKey,
    __out      DEVPROPTYPE *PropertyType,
    __out_opt  PBYTE PropertyBuffer,
    __in       DWORD PropertyBufferSize,
    __out_opt  PDWORD RequiredSize,
    __in       DWORD Flags);

  FN_SetupDiGetDevicePropertyW fn_SetupDiGetDevicePropertyW = (FN_SetupDiGetDevicePropertyW)
        GetProcAddress(GetModuleHandle(TEXT("Setupapi.dll")), "SetupDiGetDevicePropertyW");

  if (fn_SetupDiGetDevicePropertyW (
        hDevInfo,
        &deviceInfoData,
        &DEVPKEY_Device_ContainerId,
        &ulPropertyType,
        reinterpret_cast<BYTE*>(szWUuidBuffer),
        sizeof(szWUuidBuffer),
        &dwSize,
        0)) {
    szWUuidBuffer[dwSize] = '\0';

    // Given the UUID bytes, build up a (widechar) string from it. There's some mangling
    // going on.
    StringFromGUID2((REFGUID)szWUuidBuffer, containerUuid, ARRAY_SIZE(containerUuid));
  } else {
    // Container UUID could not be fetched, return empty serial number.
    return;
  }

  // NOTE: Devices might have a containerUuid like {00000000-0000-0000-FFFF-FFFFFFFFFFFF}
  // This means they're non-removable, and are not handled (yet).
  // Maybe they should inherit the s/n from somewhere else.

  // Sanitize input - for whatever reason, StringFromGUID2() returns a WCHAR* but
  // the comparisons later need a plain old char*, in lowercase ASCII.
  char wantedUuid[MAX_BUFFER_SIZE];
  _snprintf_s(wantedUuid, MAX_BUFFER_SIZE, _TRUNCATE, "%ws", containerUuid);
  strlwr(wantedUuid);

  // Iterate through all the USB devices with the given VendorID/ProductID

  HKEY vendorProductHKey;
  DWORD retCode;
  char hkeyPath[MAX_BUFFER_SIZE];

  _snprintf_s(hkeyPath, MAX_BUFFER_SIZE, _TRUNCATE, "SYSTEM\\CurrentControlSet\\Enum\\USB\\VID_%s&PID_%s", vid, pid);

  retCode = RegOpenKeyEx(
    HKEY_LOCAL_MACHINE,
    hkeyPath,
    0,
    KEY_READ,
    &vendorProductHKey);

  if (retCode == ERROR_SUCCESS) {
    DWORD    serialNumbersCount = 0;       // number of subkeys

    // Fetch how many subkeys there are for this VendorID/ProductID pair.
    // That's the number of devices for this VendorID/ProductID known to this machine.

    retCode = RegQueryInfoKey(
        vendorProductHKey,    // hkey handle
        NULL,      // buffer for class name
        NULL,      // size of class string
        NULL,      // reserved
        &serialNumbersCount,  // number of subkeys
        NULL,      // longest subkey size
        NULL,      // longest class string
        NULL,      // number of values for this key
        NULL,      // longest value name
        NULL,      // longest value data
        NULL,      // security descriptor
        NULL);     // last write time

    if (retCode == ERROR_SUCCESS && serialNumbersCount > 0) {
        for (unsigned int i=0; i < serialNumbersCount; i++) {
          // Each of the subkeys here is the serial number of a USB device with the
          // given VendorId/ProductId. Now fetch the string for the S/N.
          DWORD serialNumberLength = maxSerialNumberLength;
          retCode = RegEnumKeyEx(vendorProductHKey,
                                  i,
                                  serialNumber,
                                  &serialNumberLength,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL);

          if (retCode == ERROR_SUCCESS) {
            // Lookup info for VID_(vendorId)&PID_(productId)\(serialnumber)

            _snprintf_s(hkeyPath, MAX_BUFFER_SIZE, _TRUNCATE,
                        "SYSTEM\\CurrentControlSet\\Enum\\USB\\VID_%s&PID_%s\\%s",
                        vid, pid, serialNumber);

            HKEY deviceHKey;

            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, hkeyPath, 0, KEY_READ, &deviceHKey) == ERROR_SUCCESS) {
                char readUuid[MAX_BUFFER_SIZE];
                DWORD readSize = sizeof(readUuid);

                // Query VID_(vendorId)&PID_(productId)\(serialnumber)\ContainerID
                DWORD retCode = RegQueryValueEx(deviceHKey, "ContainerID", NULL, NULL, (LPBYTE)&readUuid, &readSize);
                if (retCode == ERROR_SUCCESS) {
                    readUuid[readSize] = '\0';
                    if (strcmp(wantedUuid, readUuid) == 0) {
                        // The ContainerID UUIDs match, return now that serialNumber has
                        // the right value.
                        RegCloseKey(deviceHKey);
                        RegCloseKey(vendorProductHKey);
                        return;
                    }
                }
            }
            RegCloseKey(deviceHKey);
          }
       }
    }

    /* In case we did not obtain the path, for whatever reason, we close the key and return an empty string. */
    RegCloseKey(vendorProductHKey);
  }

  _snprintf_s(serialNumber, maxSerialNumberLength, _TRUNCATE, "");
  return;
}

void setIfNotEmpty(v8::Local<v8::Object> item, std::string key, const char *value) {
  v8::Local<v8::String> v8key = Nan::New<v8::String>(key).ToLocalChecked();
  if (strlen(value) > 0) {
    Nan::Set(item, v8key, Nan::New<v8::String>(value).ToLocalChecked());
  } else {
    Nan::Set(item, v8key, Nan::Undefined());
  }
}

void EIO_AfterList(uv_async_t* req) {

  Nan::HandleScope scope;
  ListBaton* data = static_cast<ListBaton*>(req->data);
  uv_close(reinterpret_cast<uv_handle_t*>(req), AsyncCloseCallback);

  v8::Local<v8::Value> argv[2];
  
  if (data->errorString[0]) {
    
    argv[0] = v8::Exception::Error(Nan::New<v8::String>(data->errorString).ToLocalChecked());
    argv[1] = Nan::Undefined();

  } else {

    v8::Local<v8::Array> results = Nan::New<v8::Array>();
    int i = 0;
    for (std::list<ListResultItem*>::iterator it = data->results.begin(); it != data->results.end(); ++it, i++) {

      v8::Local<v8::Object> item = Nan::New<v8::Object>();

      setIfNotEmpty(item, "path", (*it)->path.c_str());
      setIfNotEmpty(item, "manufacturer", (*it)->manufacturer.c_str());
      setIfNotEmpty(item, "serialNumber", (*it)->serialNumber.c_str());
      setIfNotEmpty(item, "pnpId", (*it)->pnpId.c_str());
      setIfNotEmpty(item, "locationId", (*it)->locationId.c_str());
      setIfNotEmpty(item, "vendorId", (*it)->vendorId.c_str());
      setIfNotEmpty(item, "productId", (*it)->productId.c_str());

      Nan::Set(results, i, item);
    }

    argv[0] = Nan::Null();
    argv[1] = results;
  }

  data->callback.Call(2, argv, data);

  for (std::list<ListResultItem*>::iterator it = data->results.begin(); it != data->results.end(); ++it) {
    delete *it;
  }

  delete data;
}

class Lister : public Worker {
    void list(ListBaton* data) {

        GUID* guidDev = (GUID*)&GUID_DEVCLASS_PORTS;  // NOLINT
        HDEVINFO hDevInfo = SetupDiGetClassDevs(guidDev, NULL, NULL, DIGCF_PRESENT | DIGCF_PROFILE);
        SP_DEVINFO_DATA deviceInfoData;

        int memberIndex = 0;

        DWORD dwSize, dwPropertyRegDataType;
        char szBuffer[MAX_BUFFER_SIZE];
        char* pnpId;
        char* vendorId;
        char* productId;
        char* name;
        char* manufacturer;
        char* locationId;
        char serialNumber[MAX_REGISTRY_KEY_SIZE];
        bool isCom;

        auto out = logger("device-list");

        while (true) {
            pnpId = NULL;
            vendorId = NULL;
            productId = NULL;
            name = NULL;
            manufacturer = NULL;
            locationId = NULL;

            out << "loop-start " << memberIndex << "\n";
            out.flush();

            ZeroMemory(&deviceInfoData, sizeof(SP_DEVINFO_DATA));
            deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

            if (SetupDiEnumDeviceInfo(hDevInfo, memberIndex, &deviceInfoData) == FALSE) {
                auto errorNo = GetLastError();

                out << "SetupDiEnumDeviceInfo error " << errorNo << "\n";
                out.flush();

                if (errorNo == ERROR_NO_MORE_ITEMS) {
                    break;
                }

                ErrorCodeToString("Getting device info", errorNo, data->errorString);
                break;
            }

            out << "SetupDiEnumDeviceInfo success\n";
            out.flush();

            dwSize = sizeof(szBuffer);
            SetupDiGetDeviceInstanceId(hDevInfo, &deviceInfoData, szBuffer, dwSize, &dwSize);
            szBuffer[dwSize] = '\0';
            pnpId = strdup(szBuffer);

            out << "SetupDiGetDeviceInstanceId success\n";
            out.flush();

            vendorId = strstr(szBuffer, "VID_");
            if (vendorId) {
                vendorId += 4;
                vendorId = copySubstring(vendorId, 4);
            }
            productId = strstr(szBuffer, "PID_");
            if (productId) {
                productId += 4;
                productId = copySubstring(productId, 4);
            }

            getSerialNumber(vendorId, productId, hDevInfo, deviceInfoData, MAX_REGISTRY_KEY_SIZE, serialNumber);

            out << "getSerialNumber success\n";
            out.flush();

            if (SetupDiGetDeviceRegistryProperty(hDevInfo, &deviceInfoData,
                SPDRP_LOCATION_INFORMATION, &dwPropertyRegDataType,
                reinterpret_cast<BYTE*>(szBuffer),
                sizeof(szBuffer), &dwSize)) {
                locationId = strdup(szBuffer);
            }

            out << "SetupDiGetDeviceRegistryProperty success\n";
            out.flush();

            if (SetupDiGetDeviceRegistryProperty(hDevInfo, &deviceInfoData,
                SPDRP_MFG, &dwPropertyRegDataType,
                reinterpret_cast<BYTE*>(szBuffer),
                sizeof(szBuffer), &dwSize)) {
                manufacturer = strdup(szBuffer);
            }

            out << "SetupDiGetDeviceRegistryProperty success\n";
            out.flush();

            HKEY hkey = SetupDiOpenDevRegKey(hDevInfo, &deviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);

            if (hkey != INVALID_HANDLE_VALUE) {
                dwSize = sizeof(szBuffer);
                if (RegQueryValueEx(hkey, "PortName", NULL, NULL, (LPBYTE)&szBuffer, &dwSize) == ERROR_SUCCESS) {
                    szBuffer[dwSize] = '\0';
                    name = strdup(szBuffer);
                    isCom = strstr(szBuffer, "COM") != NULL;
                }
            }

            out << "SetupDiOpenDevRegKey done\n";
            out.flush();

            if (isCom) {
                ListResultItem* resultItem = new ListResultItem();
                resultItem->path = name;
                resultItem->manufacturer = manufacturer;
                resultItem->pnpId = pnpId;
                if (vendorId) {
                    resultItem->vendorId = vendorId;
                }
                if (productId) {
                    resultItem->productId = productId;
                }
                resultItem->serialNumber = serialNumber;
                if (locationId) {
                    resultItem->locationId = locationId;
                }
                data->results.push_back(resultItem);
            }

            free(pnpId);
            free(vendorId);
            free(productId);
            free(locationId);
            free(manufacturer);
            free(name);

            RegCloseKey(hkey);
            memberIndex++;

            out << "loop-end\n";
            out.flush();
        }

        if (hDevInfo) {
            out << "destroy-list\n";
            out.flush();
            SetupDiDestroyDeviceInfoList(hDevInfo);
        }

        out << "return\n";
        out.flush();
    }
public:
    void thread() {
        while (true) {
            auto async = getNextJob();
            ListBaton* baton = static_cast<ListBaton*>(async->data);

            list(baton);

            // Signal the main thread to run the callback.
            send_async(async);
        }
    }
};

Lister* lister;

NAN_METHOD(List) {
    // callback
    if (!info[0]->IsFunction()) {
        Nan::ThrowTypeError("First argument must be a function");
        return;
    }

    ListBaton* baton = new ListBaton();
    snprintf(baton->errorString, sizeof(baton->errorString), "");
    baton->callback.Reset(info[0].As<v8::Function>());

    uv_async_t* async = new uv_async_t;
    uv_async_init(uv_default_loop(), async, EIO_AfterList);
    
    async->data = baton;

    lister->addJob(async);
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

void internalInit() {
    writer = new Writer();
    for (int i = 0; i < 1; i++) {
        std::thread t(&Writer::thread, writer);
    }

    reader = new Reader();
    for (int i = 0; i < 1; i++) {
        std::thread t(&Reader::thread, reader);
    }

    lister = new Lister();
    for (int i = 0; i < 1; i++) {
        std::thread t(&Lister::thread, lister);
    }
}