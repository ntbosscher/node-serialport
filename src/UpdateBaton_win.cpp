
#include "./UpdateBaton.h"
#include "./Util.h"
#include "./Win.h"
#include "./V8ArgDecoder.h"

void UpdateBaton::run()
{

    DCB dcb = {0};
    SecureZeroMemory(&dcb, sizeof(DCB));
    dcb.DCBlength = sizeof(DCB);

    if (!GetCommState(int2handle(this->fd), &dcb))
    {
        ErrorCodeToString("Update (GetCommState)", GetLastError(), this->errorString);
        return;
    }

    dcb.BaudRate = this->baudRate;

    if (!SetCommState(int2handle(this->fd), &dcb))
    {
        ErrorCodeToString("Update (SetCommState)", GetLastError(), this->errorString);
        return;
    }
}