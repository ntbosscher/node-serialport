#include "./serialport.h"
#include "./serialport_win.h"
#include <nan.h>
#include <list>
#include <vector>
#include <string.h>
#include <windows.h>
#include <Setupapi.h>
#include <initguid.h>
#include <devpkey.h>
#include <ctime>
#include <devguid.h>
#include <fstream>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <mutex>

// It's possible that the s/n is a construct and not the s/n of the parent USB
// composite device. This performs some convoluted registry lookups to fetch the USB s/n.
void getSerialNumber(const char *vid,
                     const char *pid,
                     const HDEVINFO hDevInfo,
                     SP_DEVINFO_DATA deviceInfoData,
                     const unsigned int maxSerialNumberLength,
                     char* serialNumber) {
    _snprintf_s(serialNumber, maxSerialNumberLength, _TRUNCATE, "");
    if (vid == NULL || pid == NULL) {
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
    typedef BOOL (WINAPI *FN_SetupDiGetDevicePropertyW)(
                                                        __in       HDEVINFO DeviceInfoSet,
                                                        __in       PSP_DEVINFO_DATA DeviceInfoData,
                                                        __in       const DEVPROPKEY *PropertyKey,
                                                        __out      DEVPROPTYPE *PropertyType,
                                                        __out_opt  PBYTE PropertyBuffer,
                                                        __in       DWORD PropertyBufferSize,
                                                        __out_opt  PDWORD RequiredSize,
                                                        __in       DWORD Flags);
    
    FN_SetupDiGetDevicePropertyW fn_SetupDiGetDevicePropertyW = (FN_SetupDiGetDevicePropertyW)
    GetProcAddress(GetModuleHandle(TEXT("Setupapi.dll")), "SetupDiGetDevicePropertyW");
    
    if (fn_SetupDiGetDevicePropertyW (
                                      hDevInfo,
                                      &deviceInfoData,
                                      &DEVPKEY_Device_ContainerId,
                                      &ulPropertyType,
                                      reinterpret_cast<BYTE*>(szWUuidBuffer),
                                      sizeof(szWUuidBuffer),
                                      &dwSize,
                                      0)) {
        szWUuidBuffer[dwSize] = '\0';
        
        // Given the UUID bytes, build up a (widechar) string from it. There's some mangling
        // going on.
        StringFromGUID2((REFGUID)szWUuidBuffer, containerUuid, ARRAY_SIZE(containerUuid));
    } else {
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
    
    if (retCode == ERROR_SUCCESS) {
        DWORD    serialNumbersCount = 0;       // number of subkeys
        
        // Fetch how many subkeys there are for this VendorID/ProductID pair.
        // That's the number of devices for this VendorID/ProductID known to this machine.
        
        retCode = RegQueryInfoKey(
                                  vendorProductHKey,    // hkey handle
                                  NULL,      // buffer for class name
                                  NULL,      // size of class string
                                  NULL,      // reserved
                                  &serialNumbersCount,  // number of subkeys
                                  NULL,      // longest subkey size
                                  NULL,      // longest class string
                                  NULL,      // number of values for this key
                                  NULL,      // longest value name
                                  NULL,      // longest value data
                                  NULL,      // security descriptor
                                  NULL);     // last write time
        
        if (retCode == ERROR_SUCCESS && serialNumbersCount > 0) {
            for (unsigned int i=0; i < serialNumbersCount; i++) {
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
                
                if (retCode == ERROR_SUCCESS) {
                    // Lookup info for VID_(vendorId)&PID_(productId)\(serialnumber)
                    
                    _snprintf_s(hkeyPath, MAX_BUFFER_SIZE, _TRUNCATE,
                                "SYSTEM\\CurrentControlSet\\Enum\\USB\\VID_%s&PID_%s\\%s",
                                vid, pid, serialNumber);
                    
                    HKEY deviceHKey;
                    
                    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, hkeyPath, 0, KEY_READ, &deviceHKey) == ERROR_SUCCESS) {
                        char readUuid[MAX_BUFFER_SIZE];
                        DWORD readSize = sizeof(readUuid);
                        
                        // Query VID_(vendorId)&PID_(productId)\(serialnumber)\ContainerID
                        DWORD retCode = RegQueryValueEx(deviceHKey, "ContainerID", NULL, NULL, (LPBYTE)&readUuid, &readSize);
                        if (retCode == ERROR_SUCCESS) {
                            readUuid[readSize] = '\0';
                            if (strcmp(wantedUuid, readUuid) == 0) {
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

class ListBaton : public BatonBase {
public:
    std::list<ListResultItem*> results;
    
    ListBaton(v8::Local<v8::Function> callback_) : BatonBase("node-serialport:ListBaton", callback_)
    {
        request.data = this;
    }
    
    override void run() {
        
        GUID* guidDev = (GUID*)&GUID_DEVCLASS_PORTS;  // NOLINT
        HDEVINFO hDevInfo = SetupDiGetClassDevs(guidDev, NULL, NULL, DIGCF_PRESENT | DIGCF_PROFILE);
        SP_DEVINFO_DATA deviceInfoData;
        
        int memberIndex = 0;
        
        DWORD dwSize, dwPropertyRegDataType;
        char szBuffer[MAX_BUFFER_SIZE];
        char* pnpId;
        char* vendorId;
        char* productId;
        char* name;
        char* manufacturer;
        char* locationId;
        char serialNumber[MAX_REGISTRY_KEY_SIZE];
        bool isCom;
        
        while (true) {
            pnpId = NULL;
            vendorId = NULL;
            productId = NULL;
            name = NULL;
            manufacturer = NULL;
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
            
            out << "SetupDiGetDeviceInstanceId success\n";
            out.flush();
            
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
            
            out << "getSerialNumber success\n";
            out.flush();
            
            if (SetupDiGetDeviceRegistryProperty(hDevInfo, &deviceInfoData,
                                                 SPDRP_LOCATION_INFORMATION, &dwPropertyRegDataType,
                                                 reinterpret_cast<BYTE*>(szBuffer),
                                                 sizeof(szBuffer), &dwSize)) {
                locationId = strdup(szBuffer);
            }
            
            out << "SetupDiGetDeviceRegistryProperty success\n";
            out.flush();
            
            if (SetupDiGetDeviceRegistryProperty(hDevInfo, &deviceInfoData,
                                                 SPDRP_MFG, &dwPropertyRegDataType,
                                                 reinterpret_cast<BYTE*>(szBuffer),
                                                 sizeof(szBuffer), &dwSize)) {
                manufacturer = strdup(szBuffer);
            }
            
            out << "SetupDiGetDeviceRegistryProperty success\n";
            out.flush();
            
            HKEY hkey = SetupDiOpenDevRegKey(hDevInfo, &deviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
            
            if (hkey != INVALID_HANDLE_VALUE) {
                dwSize = sizeof(szBuffer);
                if (RegQueryValueEx(hkey, "PortName", NULL, NULL, (LPBYTE)&szBuffer, &dwSize) == ERROR_SUCCESS) {
                    szBuffer[dwSize] = '\0';
                    name = strdup(szBuffer);
                    isCom = strstr(szBuffer, "COM") != NULL;
                }
            }
            
            out << "SetupDiOpenDevRegKey done\n";
            out.flush();
            
            if (isCom) {
                ListResultItem* resultItem = new ListResultItem();
                resultItem->path = name;
                resultItem->manufacturer = manufacturer;
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
            }
            
            free(pnpId);
            free(vendorId);
            free(productId);
            free(locationId);
            free(manufacturer);
            free(name);
            
            RegCloseKey(hkey);
            memberIndex++;
            
            out << "loop-end\n";
            out.flush();
        }
        
        if (hDevInfo) {
            out << "destroy-list\n";
            out.flush();
            SetupDiDestroyDeviceInfoList(hDevInfo);
        }
        
        out << "return\n";
        out.flush();
    }
    
    override v8::Local<v8::Value> getReturnValue() {
        
        v8::Local<v8::Array> results = Nan::New<v8::Array>();
        int i = 0;
        for (std::list<ListResultItem*>::iterator it = data->results.begin(); it != data->results.end(); ++it, i++) {
            
            v8::Local<v8::Object> item = Nan::New<v8::Object>();
            
            setIfNotEmpty(item, "path", (*it)->path.c_str());
            setIfNotEmpty(item, "manufacturer", (*it)->manufacturer.c_str());
            setIfNotEmpty(item, "serialNumber", (*it)->serialNumber.c_str());
            setIfNotEmpty(item, "pnpId", (*it)->pnpId.c_str());
            setIfNotEmpty(item, "locationId", (*it)->locationId.c_str());
            setIfNotEmpty(item, "vendorId", (*it)->vendorId.c_str());
            setIfNotEmpty(item, "productId", (*it)->productId.c_str());
            
            Nan::Set(results, i, item);
        }
        
        for (std::list<ListResultItem*>::iterator it = data->results.begin(); it != data->results.end(); ++it) {
            delete *it;
        }
        
    }
    
    ~ListBaton() {
        callback.Reset();
    }
};

NAN_METHOD(List) {
    // callback
    if (!info[0]->IsFunction()) {
        Nan::ThrowTypeError("First argument must be a function");
        return;
    }
    
    v8::Local<v8::Function> cb = Nan::To<v8::Function>(info[0]).ToLocalChecked();
    
    ListBaton* baton = new ListBaton(cb);
    baton->start();
}

