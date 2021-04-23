
#include "./UpdateBaton.h"
#include "./Darwin.h"

void UpdateBaton::run() {
    ConnectionOptions* connectionOptions = new ConnectionOptions();
    connectionOptions->fd = fd;
    connectionOptions->baudRate = baudRate;

    setBaudRate(connectionOptions);
    delete connectionOptions;
}