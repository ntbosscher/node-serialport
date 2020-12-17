
#include "./OpenBaton.h"
#include "./util.h"
#include "./V8ArgDecoder.h"

v8::Local<v8::Value> OpenBaton::getReturnValue()
{
    return Nan::New<v8::Int32>(this->result);
}

void OpenBaton::run()
{
    char originalPath[1024];
    strncpy_s(originalPath, sizeof(originalPath), this->path, _TRUNCATE);
    // this->path is char[1024] but on Windows it has the form "COMx\0" or "COMxx\0"
    // We want to prepend "\\\\.\\" to it before we call CreateFile
    strncpy(this->path + 20, this->path, 10);
    strncpy(this->path, "\\\\.\\", 4);
    strncpy(this->path + 4, this->path + 20, 10);

    int shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    if (this->lock)
    {
        shareMode = 0;
    }

    HANDLE file = CreateFile(
        this->path,
        GENERIC_READ | GENERIC_WRITE,
        shareMode, // dwShareMode 0 Prevents other processes from opening if they request delete, read, or write access
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED, // allows for reading and writing at the same time and sets the handle for asynchronous I/O
        NULL);

    if (file == INVALID_HANDLE_VALUE)
    {
        DWORD errorCode = GetLastError();
        char temp[100];
        _snprintf_s(temp, sizeof(temp), _TRUNCATE, "Opening %s", originalPath);
        ErrorCodeToString(temp, errorCode, this->errorString);
        return;
    }

    DCB dcb = {0};
    SecureZeroMemory(&dcb, sizeof(DCB));
    dcb.DCBlength = sizeof(DCB);

    if (!GetCommState(file, &dcb))
    {
        ErrorCodeToString("Open (GetCommState)", GetLastError(), this->errorString);
        CloseHandle(file);
        return;
    }

    if (this->hupcl)
    {
        dcb.fDtrControl = DTR_CONTROL_ENABLE;
    }
    else
    {
        dcb.fDtrControl = DTR_CONTROL_DISABLE; // disable DTR to avoid reset
    }

    dcb.Parity = NOPARITY;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;

    dcb.fOutxDsrFlow = FALSE;
    dcb.fOutxCtsFlow = FALSE;

    if (this->xon)
    {
        dcb.fOutX = TRUE;
    }
    else
    {
        dcb.fOutX = FALSE;
    }

    if (this->xoff)
    {
        dcb.fInX = TRUE;
    }
    else
    {
        dcb.fInX = FALSE;
    }

    if (this->rtscts)
    {
        dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
        dcb.fOutxCtsFlow = TRUE;
    }
    else
    {
        dcb.fRtsControl = RTS_CONTROL_DISABLE;
    }

    dcb.fBinary = true;
    dcb.BaudRate = this->baudRate;
    dcb.ByteSize = this->dataBits;

    switch (this->parity)
    {
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

    switch (this->stopBits)
    {
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

    if (!SetCommState(file, &dcb))
    {
        ErrorCodeToString("Open (SetCommState)", GetLastError(), this->errorString);
        CloseHandle(file);
        return;
    }

    // Set the timeouts for read and write operations.
    // Read operation will wait for at least 1 byte to be received.
    COMMTIMEOUTS commTimeouts = {};
    commTimeouts.ReadIntervalTimeout = 0;          // Never timeout, always wait for data.
    commTimeouts.ReadTotalTimeoutMultiplier = 0;   // Do not allow big read timeout when big read buffer used
    commTimeouts.ReadTotalTimeoutConstant = 1000;  // Total read timeout (period of read loop)
    commTimeouts.WriteTotalTimeoutConstant = 1000; // Const part of write timeout
    commTimeouts.WriteTotalTimeoutMultiplier = 0;  // Variable part of write timeout (per byte)

    if (!SetCommTimeouts(file, &commTimeouts))
    {
        ErrorCodeToString("Open (SetCommTimeouts)", GetLastError(), this->errorString);
        CloseHandle(file);
        return;
    }

    // Remove garbage data in RX/TX queues
    PurgeComm(file, PURGE_RXCLEAR);
    PurgeComm(file, PURGE_TXCLEAR);

    this->result = static_cast<int>(reinterpret_cast<uintptr_t>(file));
}

NAN_METHOD(Open)
{
    V8ArgDecoder args(&info);

    auto path = args.takeString();
    auto object = args.takeObject();
    auto cb = args.takeFunction();

    if(args.hasError()) return;

    OpenBaton *baton = new OpenBaton("OpenBaton", cb);
    strncpy_s(baton->path, sizeof(baton->path), path.c_str(), _TRUNCATE);
    baton->baudRate = object.getInt("baudRate");
    baton->dataBits = object.getInt("dataBits");
    baton->parity = ToParityEnum(object.getV8String("parity"));
    baton->stopBits = ToStopBitEnum(object.getDouble("stopBits"));
    baton->rtscts = object.getBool("rtscts");
    baton->xon = object.getBool("xon");
    baton->xoff = object.getBool("xoff");
    baton->xany = object.getBool("xany");
    baton->hupcl = object.getBool("hupcl");
    baton->lock = object.getBool("lock");

#ifndef WIN32
    baton->vmin = object.getInt("vmin");
    baton->vtime = object.getInt("vtime");
#endif

    baton->start();
}