
#include "./OpenBaton.h"
#include <IOKit/serial/ioss.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "./Darwin.h"

int ToDataBitsConstant(int dataBits) {
    switch (dataBits) {
        case 8: default: return CS8;
        case 7: return CS7;
        case 6: return CS6;
        case 5: return CS5;
    }
    return -1;
}

int setup(int fd, OpenBaton *data) {

    int dataBits = ToDataBitsConstant(data->dataBits);
    if (-1 == dataBits) {
        snprintf(data->errorString, sizeof(data->errorString),
                 "Invalid data bits setting %d", data->dataBits);
        return -1;
    }

    // Snow Leopard doesn't have O_CLOEXEC
    if (-1 == fcntl(fd, F_SETFD, FD_CLOEXEC)) {
        snprintf(data->errorString, sizeof(data->errorString), "Error %s Cannot open %s", strerror(errno), data->path);
        return -1;
    }

    // Get port configuration for modification
    struct termios options;
    tcgetattr(fd, &options);

    // IGNPAR: ignore bytes with parity errors
    options.c_iflag = IGNPAR;

    // ICRNL: map CR to NL (otherwise a CR input on the other computer will not terminate input)
    // Future potential option
    // options.c_iflag = ICRNL;
    // otherwise make device raw (no other input processing)

    // Specify data bits
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= dataBits;

    options.c_cflag &= ~(CRTSCTS);

    if (data->rtscts) {
        options.c_cflag |= CRTSCTS;
        // evaluate specific flow control options
    }

    options.c_iflag &= ~(IXON | IXOFF | IXANY);

    if (data->xon) {
        options.c_iflag |= IXON;
    }

    if (data->xoff) {
        options.c_iflag |= IXOFF;
    }

    if (data->xany) {
        options.c_iflag |= IXANY;
    }

    switch (data->parity) {
        case SERIALPORT_PARITY_NONE:
            options.c_cflag &= ~PARENB;
            // options.c_cflag &= ~CSTOPB;
            // options.c_cflag &= ~CSIZE;
            // options.c_cflag |= CS8;
            break;
        case SERIALPORT_PARITY_ODD:
            options.c_cflag |= PARENB;
            options.c_cflag |= PARODD;
            // options.c_cflag &= ~CSTOPB;
            // options.c_cflag &= ~CSIZE;
            // options.c_cflag |= CS7;
            break;
        case SERIALPORT_PARITY_EVEN:
            options.c_cflag |= PARENB;
            options.c_cflag &= ~PARODD;
            // options.c_cflag &= ~CSTOPB;
            // options.c_cflag &= ~CSIZE;
            // options.c_cflag |= CS7;
            break;
        default:
            snprintf(data->errorString, sizeof(data->errorString), "Invalid parity setting %d", data->parity);
            return -1;
    }

    switch (data->stopBits) {
        case SERIALPORT_STOPBITS_ONE:
            options.c_cflag &= ~CSTOPB;
            break;
        case SERIALPORT_STOPBITS_TWO:
            options.c_cflag |= CSTOPB;
            break;
        default:
            snprintf(data->errorString, sizeof(data->errorString), "Invalid stop bits setting %d", data->stopBits);
            return -1;
    }

    options.c_cflag |= CLOCAL;  // ignore status lines
    options.c_cflag |= CREAD;   // enable receiver
    if (data->hupcl) {
        options.c_cflag |= HUPCL;  // drop DTR (i.e. hangup) on close
    }

    // Raw output
    options.c_oflag = 0;

    // ICANON makes partial lines not readable. It should be optional.
    // It works with ICRNL.
    options.c_lflag = 0;  // ICANON;
    options.c_cc[VMIN]= data->vmin;
    options.c_cc[VTIME]= data->vtime;

    // Note that tcsetattr() returns success if any of the requested changes could be successfully carried out.
    // Therefore, when making multiple changes it may be necessary to follow this call with a further call to
    // tcgetattr() to check that all changes have been performed successfully.
    // This also fails on OSX
    tcsetattr(fd, TCSANOW, &options);

    if (data->lock) {
        if (-1 == flock(fd, LOCK_EX | LOCK_NB)) {
            snprintf(data->errorString, sizeof(data->errorString), "Error %s Cannot lock port", strerror(errno));
            return -1;
        }
    }

    // Copy the connection options into the ConnectionOptionsBaton to set the baud rate
    ConnectionOptions* connectionOptions = new ConnectionOptions();
    connectionOptions->fd = fd;
    connectionOptions->baudRate = data->baudRate;

    if (-1 == setBaudRate(connectionOptions)) {
        strncpy(data->errorString, connectionOptions->errorString, sizeof(data->errorString));
        delete(connectionOptions);
        return -1;
    }
    delete(connectionOptions);

    // flush all unread and wrote data up to this point because it could have been received or sent with bad settings
    // Not needed since setBaudRate does this for us
    // tcflush(fd, TCIOFLUSH);

    return 1;
}

void OpenBaton::run()
{

    int flags = (O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC | O_SYNC);
    int fd = open(this->path, flags);

    if (-1 == fd) {
        snprintf(this->errorString, sizeof(this->errorString), "Error: %s, cannot open %s", strerror(errno), this->path);
        return;
    }

    if (-1 == setup(fd, this)) {
        close(fd);
        return;
    }

    this->result = fd;
}