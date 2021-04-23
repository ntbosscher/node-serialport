#include <IOKit/serial/ioss.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "./Darwin.h"

int ToBaudConstant(int baudRate) {
    switch (baudRate) {
        case 0: return B0;
        case 50: return B50;
        case 75: return B75;
        case 110: return B110;
        case 134: return B134;
        case 150: return B150;
        case 200: return B200;
        case 300: return B300;
        case 600: return B600;
        case 1200: return B1200;
        case 1800: return B1800;
        case 2400: return B2400;
        case 4800: return B4800;
        case 9600: return B9600;
        case 19200: return B19200;
        case 38400: return B38400;
        case 57600: return B57600;
        case 115200: return B115200;
        case 230400: return B230400;
#if defined(__linux__)
            case 460800: return B460800;
    case 500000: return B500000;
    case 576000: return B576000;
    case 921600: return B921600;
    case 1000000: return B1000000;
    case 1152000: return B1152000;
    case 1500000: return B1500000;
    case 2000000: return B2000000;
    case 2500000: return B2500000;
    case 3000000: return B3000000;
    case 3500000: return B3500000;
    case 4000000: return B4000000;
#endif
    }
    return -1;
}

int setBaudRate(ConnectionOptions *data) {
    // lookup the standard baudrates from the table
    int baudRate = ToBaudConstant(data->baudRate);
    int fd = data->fd;

    // get port options
    struct termios options;
    if (-1 == tcgetattr(fd, &options)) {
        snprintf(data->errorString, sizeof(data->errorString),
                 "Error: %s setting custom baud rate of %d", strerror(errno), data->baudRate);
        return -1;
    }

    // If there is a custom baud rate on linux you can do the following trick with B38400
#if defined(__linux__) && defined(ASYNC_SPD_CUST)
    if (baudRate == -1) {
      int err = linuxSetCustomBaudRate(fd, data->baudRate);

      if (err == -1) {
        snprintf(data->errorString, sizeof(data->errorString),
                 "Error: %s || while retrieving termios2 info", strerror(errno));
        return -1;
      } else if (err == -2) {
        snprintf(data->errorString, sizeof(data->errorString),
                 "Error: %s || while setting custom baud rate of %d", strerror(errno), data->baudRate);
        return -1;
      }

      return 1;
    }
#endif

    // On OS X, starting with Tiger, we can set a custom baud rate with ioctl
#if defined(MAC_OS_X_VERSION_10_4) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4)
    if (-1 == baudRate) {
      speed_t speed = data->baudRate;
      if (-1 == ioctl(fd, IOSSIOSPEED, &speed)) {
        snprintf(data->errorString, sizeof(data->errorString),
                 "Error: %s calling ioctl(.., IOSSIOSPEED, %ld )", strerror(errno), speed);
        return -1;
      } else {
        tcflush(fd, TCIOFLUSH);
        return 1;
      }
    }
#endif

    if (-1 == baudRate) {
        snprintf(data->errorString, sizeof(data->errorString),
                 "Error baud rate of %d is not supported on your platform", data->baudRate);
        return -1;
    }

    // If we have a good baud rate set it and lets go
    cfsetospeed(&options, baudRate);
    cfsetispeed(&options, baudRate);
    // throw away all the buffered data
    tcflush(fd, TCIOFLUSH);
    // make the changes now
    tcsetattr(fd, TCSANOW, &options);
    return 1;
}