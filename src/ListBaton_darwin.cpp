
#include "./ListUtils_darwin.h"
#include "./ListBaton.h"

void ListBaton::run() {
    stDeviceListItem* devices = GetSerialDevices();

    if (devices != NULL && *(devices->length) > 0) {
        stDeviceListItem* next = devices;

        for (int i = 0, len = *(devices->length); i < len; i++) {
            stSerialDevice device = (* next).value;

            auto resultItem = std::make_unique<ListResultItem>();
            resultItem->path = device.port;

            if (*device.locationId) {
                resultItem->locationId = device.locationId;
            }
            if (*device.vendorId) {
                resultItem->vendorId = device.vendorId;
            }
            if (*device.productId) {
                resultItem->productId = device.productId;
            }
            if (*device.manufacturer) {
                resultItem->manufacturer = device.manufacturer;
            }
            if (*device.serialNumber) {
                resultItem->serialNumber = device.serialNumber;
            }

            results.push_back(std::move(resultItem));

            stDeviceListItem* current = next;

            if (next->next != NULL) {
                next = next->next;
            }

            free(current);
        }
    }
}