
#include "./SetBaton.h"
#include <sys/ioctl.h>

void SetBaton::run() {
    int bits;
    ioctl(this->fd, TIOCMGET, &bits);

    bits &= ~(TIOCM_RTS | TIOCM_CTS | TIOCM_DTR | TIOCM_DSR);

    if (this->rts) {
        bits |= TIOCM_RTS;
    }

    if (this->cts) {
        bits |= TIOCM_CTS;
    }

    if (this->dtr) {
        bits |= TIOCM_DTR;
    }

    if (this->dsr) {
        bits |= TIOCM_DSR;
    }

#if defined(__linux__)
    int err = linuxSetLowLatencyMode(this->fd, this->lowLatency);
  if (err == -1) {
    snprintf(this->errorString, sizeof(this->errorString), "Error: %s, cannot get", strerror(errno));
    return;
  } else if(err == -2) {
    snprintf(this->errorString, sizeof(this->errorString), "Error: %s, cannot set", strerror(errno));
    return;
  }
#endif

    int result = 0;
    if (this->brk) {
        result = ioctl(this->fd, TIOCSBRK, NULL);
    } else {
        result = ioctl(this->fd, TIOCCBRK, NULL);
    }

    if (-1 == result) {
        snprintf(this->errorString, sizeof(this->errorString), "Error: %s, cannot set", strerror(errno));
        return;
    }

    if (-1 == ioctl(this->fd, TIOCMSET, &bits)) {
        snprintf(this->errorString, sizeof(this->errorString), "Error: %s, cannot set", strerror(errno));
        return;
    }
}