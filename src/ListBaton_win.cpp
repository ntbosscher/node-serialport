#include "./SerialPort.h"
#include "./Win.h"
#include "./Util.h"
#include "./BatonBase.h"
#include <nan.h>
#include <setupapi.h>
#include <devguid.h>
#include <Devpkey.h>
#include <usbspec.h>
#include <Usbioctl.h>
#include <cfgmgr32.h>
#include <subauth.h>
#include <strsafe.h>
#include <initguid.h>
#include <usbiodef.h>
#include <WinBase.h>
#include "V8ArgDecoder.h"
#include "./ListBaton.h"
/* DEFINES */

#define ALLOC(dwBytes) GlobalAlloc(GPTR, (dwBytes))
#define FREE(hMem) GlobalFree((hMem))

/* f18a0e88-c30c-11d0-8815-00a0c906bed8 */
DEFINE_GUID(GUID_DEVINTERFACE_USB_HUB, 0xf18a0e88, 0xc30c, 0x11d0, 0x88, 0x15, 0x00,
            0xa0, 0xc9, 0x06, 0xbe, 0xd8);

/* A5DCBF10-6530-11D2-901F-00C04FB951ED */
DEFINE_GUID(GUID_DEVINTERFACE_USB_DEVICE, 0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00,
            0xC0, 0x4F, 0xB9, 0x51, 0xED);

/* TYPEDEFS */

typedef struct _STRING_DESCRIPTOR_NODE
{
    struct _STRING_DESCRIPTOR_NODE *Next;
    UCHAR DescriptorIndex;
    USHORT LanguageID;
    USB_STRING_DESCRIPTOR StringDescriptor[1];
} STRING_DESCRIPTOR_NODE, *PSTRING_DESCRIPTOR_NODE;

/* UTIL METHODS */

/**
 * GetInterfacePaths finds the CreateFile compatible paths for the device and 
 * class-guid provided.
 * 
 * Typically *paths only returns 1 string, but it's possible that it will return 
 * multiple paths.
 */
int GetInterfacePaths(TCHAR *sysDeviceID, LPGUID interfaceGUID, PSTR *paths)
{
    CONFIGRET cres;
    if (!paths)
        return -1;

    // Get list size
    ULONG bufSz = 0;
    cres = CM_Get_Device_Interface_List_Size(&bufSz, interfaceGUID, sysDeviceID, CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cres != CR_SUCCESS)
        return -12;

    // Allocate memory for the list
    PSTR buf = (PSTR)malloc(bufSz * sizeof(CHAR) * 2);

    // Populate the list
    cres = CM_Get_Device_Interface_List(interfaceGUID, sysDeviceID, (PZZSTR)buf, bufSz, CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cres != CR_SUCCESS)
    {
        free(paths);
        return -13;
    }

    *paths = buf;
    return 0;
}

/**
 * GetDeviceDescriptorForPortIndex finds the USB_DEVICE_DESCRIPTOR for the given usb device.
 * 
 * handle = a usb hub handle created with CreateFile()
 * portIndex = which child of the usb hub to inspect
 * descriptor = returned USB_DEVICE_DESCRIPTOR
 * 
 * returns 0 if successful
 */
int GetDeviceDescriptorForPortIndex(HANDLE handle, ULONG portIndex, USB_DEVICE_DESCRIPTOR *descriptor)
{

    PUSB_NODE_CONNECTION_INFORMATION_EX ConnectionInfo =
        (PUSB_NODE_CONNECTION_INFORMATION_EX)
            malloc(sizeof(USB_NODE_CONNECTION_INFORMATION_EX));

    ConnectionInfo->ConnectionIndex = portIndex;

    ULONG nBytes = 0;
    BOOL success = DeviceIoControl(handle,
                                   IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX,
                                   ConnectionInfo,
                                   sizeof(USB_NODE_CONNECTION_INFORMATION_EX),
                                   ConnectionInfo,
                                   sizeof(USB_NODE_CONNECTION_INFORMATION_EX),
                                   &nBytes,
                                   NULL);

    if (success)
    {
        *descriptor = ConnectionInfo->DeviceDescriptor;
        FREE(ConnectionInfo);
        return 0;
    }

    FREE(ConnectionInfo);

    PUSB_NODE_CONNECTION_INFORMATION connectionInfo = NULL;

    // Try using IOCTL_USB_GET_NODE_CONNECTION_INFORMATION
    // instead of IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX
    //

    nBytes = sizeof(USB_NODE_CONNECTION_INFORMATION) +
             sizeof(USB_PIPE_INFO) * 30;

    connectionInfo = (PUSB_NODE_CONNECTION_INFORMATION)ALLOC(nBytes);

    if (connectionInfo == NULL)
    {
        return -1;
    }

    connectionInfo->ConnectionIndex = portIndex;

    success = DeviceIoControl(handle,
                              IOCTL_USB_GET_NODE_CONNECTION_INFORMATION,
                              connectionInfo,
                              nBytes,
                              connectionInfo,
                              nBytes,
                              &nBytes,
                              NULL);

    if (!success)
    {
        FREE(connectionInfo);
        return -2;
    }

    *descriptor = connectionInfo->DeviceDescriptor;
    free(connectionInfo);

    return 0;
}

int GetStringDescriptor(
    HANDLE handle,
    ULONG connectionIndex,
    UCHAR descriptorIndex,
    wchar_t **descriptorString)
{
    int languageId = 0x0409;
    BOOL success = 0;
    ULONG nBytes = 0;
    ULONG nBytesReturned = 0;

    UCHAR stringDescReqBuf[sizeof(USB_DESCRIPTOR_REQUEST) +
                           MAXIMUM_USB_STRING_LENGTH];

    PUSB_DESCRIPTOR_REQUEST stringDescReq = NULL;
    PUSB_STRING_DESCRIPTOR stringDesc = NULL;

    nBytes = sizeof(stringDescReqBuf);

    stringDescReq = (PUSB_DESCRIPTOR_REQUEST)stringDescReqBuf;
    stringDesc = (PUSB_STRING_DESCRIPTOR)(stringDescReq + 1);

    // Zero fill the entire request structure
    //
    memset(stringDescReq, 0, nBytes);

    // Indicate the port from which the descriptor will be requested
    //
    stringDescReq->ConnectionIndex = connectionIndex;

    //
    // USBHUB uses URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE to process this
    // IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION request.
    //
    // USBD will automatically initialize these fields:
    //     bmRequest = 0x80
    //     bRequest  = 0x06
    //
    // We must inititialize these fields:
    //     wValue    = Descriptor Type (high) and Descriptor Index (low byte)
    //     wIndex    = Zero (or Language ID for String Descriptors)
    //     wLength   = Length of descriptor buffer
    //
    stringDescReq->SetupPacket.wValue = (USB_STRING_DESCRIPTOR_TYPE << 8) | descriptorIndex;

    stringDescReq->SetupPacket.wIndex = languageId;

    stringDescReq->SetupPacket.wLength = (USHORT)(nBytes - sizeof(USB_DESCRIPTOR_REQUEST));

    // Now issue the get descriptor request.
    //
    success = DeviceIoControl(handle,
                              IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
                              stringDescReq,
                              nBytes,
                              stringDescReq,
                              nBytes,
                              &nBytesReturned,
                              NULL);

    //
    // Do some sanity checks on the return from the get descriptor request.
    //

    if (!success)
    {
        return -1;
    }

    if (nBytesReturned < 2)
    {
        return -2;
    }

    if (stringDesc->bDescriptorType != USB_STRING_DESCRIPTOR_TYPE)
    {
        return -3;
    }

    if (stringDesc->bLength != nBytesReturned - sizeof(USB_DESCRIPTOR_REQUEST))
    {
        return -4;
    }

    if (stringDesc->bLength % 2 != 0)
    {
        return -5;
    }

    wchar_t *buf = (wchar_t *)ALLOC(stringDesc->bLength);
    memcpy(buf,
           stringDesc->bString,
           stringDesc->bLength);

    *descriptorString = buf;

    return 0;
}

/**
 * Fetches the iManufacturer value for the given device
 * 
 * device = the device information from the enumerator
 * portIndex = the index of the device relative to it's parent hub
 * manufacturer = the result
 * 
 * returns 0 if successful
 */
int GetiManufacturer(SP_DEVINFO_DATA device, int portIndex, char **manufacturer)
{
    DWORD deviceId = device.DevInst;
    PSTR path;

    for (int i = 0; i < 3; i++)
    {
        unsigned long len = 1024;
        byte *buf = new byte[len];

        TCHAR szDeviceInstanceID[MAX_DEVICE_ID_LEN];

        int result = CM_Get_Device_ID(deviceId, szDeviceInstanceID, ARRAY_SIZE(szDeviceInstanceID), 0);
        if (result != CR_SUCCESS)
        {
            return -1;
        }

        PSTR paths;
        LPGUID guid = (GUID *)&GUID_DEVINTERFACE_USB_HUB;

        result = GetInterfacePaths(szDeviceInstanceID, guid, &paths);
        if (result < 0)
        {
            return -2;
        }

        if (strlen(paths) == 0)
        {
            // try again one tier up
            DWORD parent;

            int result = CM_Get_Parent(&parent, deviceId, 0);
            if (result != CR_SUCCESS)
            {
                return -3;
            }

            deviceId = parent;
            continue;
        }

        // alloc on the stack so we can forget about cleaning up paths
        // and assume that there's only 1 path returned
        path = new char[strlen(paths) + 1];
        strcpy(path, paths);
        delete[] paths;

        break;
    }

    HANDLE handle = CreateFile(
        (LPCSTR)path,
        GENERIC_WRITE | GENERIC_READ,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        NULL,
        NULL);

    if (handle == INVALID_HANDLE_VALUE)
        return -2;

    USB_DEVICE_DESCRIPTOR descriptor;

    int result = GetDeviceDescriptorForPortIndex(handle, portIndex, &descriptor);
    if (result < 0)
    {
        CloseHandle(handle);
        return -4;
    }

    if (!descriptor.iManufacturer)
    {
        CloseHandle(handle);
        return -5;
    }

    wchar_t *descriptorString;
    result = GetStringDescriptor(handle,
                                 portIndex,
                                 descriptor.iManufacturer,
                                 &descriptorString);

    if (result < 0)
    {
        CloseHandle(handle);
        return result;
    }

    *manufacturer = strdup(wStr2Char(descriptorString).c_str());

    free(descriptorString);
    CloseHandle(handle);

    return 0;
}

// It's possible that the s/n is a construct and not the s/n of the parent USB
// composite device. This performs some convoluted registry lookups to fetch the USB s/n.
void getSerialNumber(const char *vid,
                     const char *pid,
                     const HDEVINFO hDevInfo,
                     SP_DEVINFO_DATA deviceInfoData,
                     const unsigned int maxSerialNumberLength,
                     char *serialNumber)
{
    _snprintf_s(serialNumber, maxSerialNumberLength, _TRUNCATE, "");
    if (vid == NULL || pid == NULL)
    {
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
    typedef BOOL(WINAPI * FN_SetupDiGetDevicePropertyW)(
        __in HDEVINFO DeviceInfoSet,
        __in PSP_DEVINFO_DATA DeviceInfoData,
        __in const DEVPROPKEY *PropertyKey,
        __out DEVPROPTYPE *PropertyType,
        __out_opt PBYTE PropertyBuffer,
        __in DWORD PropertyBufferSize,
        __out_opt PDWORD RequiredSize,
        __in DWORD Flags);

    FN_SetupDiGetDevicePropertyW fn_SetupDiGetDevicePropertyW = (FN_SetupDiGetDevicePropertyW)
        GetProcAddress(GetModuleHandle(TEXT("Setupapi.dll")), "SetupDiGetDevicePropertyW");

    if (fn_SetupDiGetDevicePropertyW(
            hDevInfo,
            &deviceInfoData,
            &DEVPKEY_Device_ContainerId,
            &ulPropertyType,
            reinterpret_cast<BYTE *>(szWUuidBuffer),
            sizeof(szWUuidBuffer),
            &dwSize,
            0))
    {
        szWUuidBuffer[dwSize] = '\0';

        // Given the UUID bytes, build up a (widechar) string from it. There's some mangling
        // going on.
        StringFromGUID2((REFGUID)szWUuidBuffer, containerUuid, ARRAY_SIZE(containerUuid));
    }
    else
    {
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

    if (retCode == ERROR_SUCCESS)
    {
        DWORD serialNumbersCount = 0; // number of subkeys

        // Fetch how many subkeys there are for this VendorID/ProductID pair.
        // That's the number of devices for this VendorID/ProductID known to this machine.

        retCode = RegQueryInfoKey(
            vendorProductHKey,   // hkey handle
            NULL,                // buffer for class name
            NULL,                // size of class string
            NULL,                // reserved
            &serialNumbersCount, // number of subkeys
            NULL,                // longest subkey size
            NULL,                // longest class string
            NULL,                // number of values for this key
            NULL,                // longest value name
            NULL,                // longest value data
            NULL,                // security descriptor
            NULL);               // last write time

        if (retCode == ERROR_SUCCESS && serialNumbersCount > 0)
        {
            for (unsigned int i = 0; i < serialNumbersCount; i++)
            {
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

                if (retCode == ERROR_SUCCESS)
                {
                    // Lookup info for VID_(vendorId)&PID_(productId)\(serialnumber)

                    _snprintf_s(hkeyPath, MAX_BUFFER_SIZE, _TRUNCATE,
                                "SYSTEM\\CurrentControlSet\\Enum\\USB\\VID_%s&PID_%s\\%s",
                                vid, pid, serialNumber);

                    HKEY deviceHKey;

                    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, hkeyPath, 0, KEY_READ, &deviceHKey) == ERROR_SUCCESS)
                    {
                        char readUuid[MAX_BUFFER_SIZE];
                        DWORD readSize = sizeof(readUuid);

                        // Query VID_(vendorId)&PID_(productId)\(serialnumber)\ContainerID
                        DWORD retCode = RegQueryValueEx(deviceHKey, "ContainerID", NULL, NULL, (LPBYTE)&readUuid, &readSize);
                        if (retCode == ERROR_SUCCESS)
                        {
                            readUuid[readSize] = '\0';
                            if (strcmp(wantedUuid, readUuid) == 0)
                            {
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

void ListBaton::run() {

    GUID *guidDev = (GUID * ) & GUID_DEVCLASS_PORTS; // NOLINT
    HDEVINFO hDevInfo = SetupDiGetClassDevs(guidDev, NULL, NULL, DIGCF_PRESENT | DIGCF_PROFILE);
    SP_DEVINFO_DATA deviceInfoData;

    int memberIndex = 0;

    DWORD dwSize, dwPropertyRegDataType;
    char szBuffer[MAX_BUFFER_SIZE];
    char *pnpId;
    char *vendorId;
    char *productId;
    char *name;
    char *manufacturer;
    char *iManufacturer;
    char *locationId;
    char serialNumber[MAX_REGISTRY_KEY_SIZE];
    bool isCom;

    while (true) {
        pnpId = NULL;
        vendorId = NULL;
        productId = NULL;
        name = NULL;
        manufacturer = NULL;
        iManufacturer = NULL;
        locationId = NULL;

        ZeroMemory(&deviceInfoData, sizeof(SP_DEVINFO_DATA));
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        if (SetupDiEnumDeviceInfo(hDevInfo, memberIndex, &deviceInfoData) == FALSE) {
            auto errorNo = GetLastError();

            if (errorNo == ERROR_NO_MORE_ITEMS) {
                break;
            }

            ErrorCodeToString("Getting device info", errorNo, errorString);
            break;
        }

        dwSize = sizeof(szBuffer);
        SetupDiGetDeviceInstanceId(hDevInfo, &deviceInfoData, szBuffer, dwSize, &dwSize);
        szBuffer[dwSize] = '\0';
        pnpId = strdup(szBuffer);

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

        if (SetupDiGetDeviceRegistryProperty(hDevInfo, &deviceInfoData,
                                             SPDRP_LOCATION_INFORMATION, &dwPropertyRegDataType,
                                             reinterpret_cast<BYTE *>(szBuffer),
                                             sizeof(szBuffer), &dwSize)) {
            locationId = strdup(szBuffer);
        }

        DWORD usbPortNumber = 0, requiredSize = 0;
        SetupDiGetDeviceRegistryProperty(hDevInfo, &deviceInfoData, SPDRP_ADDRESS, nullptr, (PBYTE) & usbPortNumber,
                                         sizeof(usbPortNumber), &requiredSize);

        GetiManufacturer(deviceInfoData, usbPortNumber, &iManufacturer);

        if (SetupDiGetDeviceRegistryProperty(hDevInfo, &deviceInfoData,
                                             SPDRP_MFG, &dwPropertyRegDataType,
                                             reinterpret_cast<BYTE *>(szBuffer),
                                             sizeof(szBuffer), &dwSize)) {
            manufacturer = strdup(szBuffer);
        }

        HKEY hkey = SetupDiOpenDevRegKey(hDevInfo, &deviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);

        if (hkey != INVALID_HANDLE_VALUE) {
            dwSize = sizeof(szBuffer);
            if (RegQueryValueEx(hkey, "PortName", NULL, NULL, (LPBYTE) & szBuffer, &dwSize) == ERROR_SUCCESS) {
                szBuffer[dwSize] = '\0';
                name = strdup(szBuffer);
                isCom = strstr(szBuffer, "COM") != NULL;
            }
        }

        if (isCom) {
            ListResultItem *resultItem = new ListResultItem();
            resultItem->path = name;

            // use iManufacturer if it's available
            if (iManufacturer != NULL) {
                resultItem->manufacturer = iManufacturer;
            } else {
                resultItem->manufacturer = manufacturer;
            }

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

            results.push_back(resultItem);
        } else {
        }

        free(pnpId);
        free(vendorId);
        free(productId);
        free(locationId);
        free(manufacturer);
        free(iManufacturer);
        free(name);

        RegCloseKey(hkey);
        memberIndex++;
    }

    if (hDevInfo) {
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }
}