#ifndef PACKAGES_SERIALPORT_SRC_SERIALPORT_H_
#define PACKAGES_SERIALPORT_SRC_SERIALPORT_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nan.h>
#include <string>
#include "./OpenBaton.h"
#include "./Util.h"


NAN_METHOD(ConfigureLogging);

int setup(int fd, OpenBaton *data);
#endif  // PACKAGES_SERIALPORT_SRC_SERIALPORT_H_
