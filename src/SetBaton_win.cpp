
#include "SetBaton.h"
#include <nan.h>
#include "Win.h"
#include "./V8ArgDecoder.h";

void SetBaton::run()
{

    if (this->rts)
    {
        EscapeCommFunction(int2handle(this->fd), SETRTS);
    }
    else
    {
        EscapeCommFunction(int2handle(this->fd), CLRRTS);
    }

    if (this->dtr)
    {
        EscapeCommFunction(int2handle(this->fd), SETDTR);
    }
    else
    {
        EscapeCommFunction(int2handle(this->fd), CLRDTR);
    }

    if (this->brk)
    {
        EscapeCommFunction(int2handle(this->fd), SETBREAK);
    }
    else
    {
        EscapeCommFunction(int2handle(this->fd), CLRBREAK);
    }

    DWORD bits = 0;

    GetCommMask(int2handle(this->fd), &bits);

    bits &= ~(EV_CTS | EV_DSR);

    if (this->cts)
    {
        bits |= EV_CTS;
    }

    if (this->dsr)
    {
        bits |= EV_DSR;
    }

    if (!SetCommMask(int2handle(this->fd), bits))
    {
        ErrorCodeToString("Setting options on COM port (SetCommMask)", GetLastError(), this->errorString);
        return;
    }
}