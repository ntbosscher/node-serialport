
# Faster SerialPort

This is a stripped down, more performant version of [node-serialport](https://github.com/serialport/node-serialport). Actually works normally with Electron.

Currently only supports windows.

## API 
```js
import FasterSerialPort from "faster-serialport";

const deviceInfos = await FasterSerialPort.list();

const deviceInfo = deviceInfos.filter(d => 
    d.path.indexOf(search) === -1 ||
    d.manufacturer.indexOf(search) === -1 ||
    d.serialNumber.indexOf(search) === -1 ||
    d.pnpId.indexOf(search) === -1 ||
    d.locationId.indexOf(search) === -1 ||
    d.vendorId.indexOf(search) === -1 ||
    d.productId.indexOf(search) === -1
)[0];

const device = new FasterSerialPort(deviceInfo.path, {
    autoOpen: false,
    baudRate: 9600,
    dataBits: 8,
    parity: "none",
    stopBits: 1,
});

await device.open();

device.setTimeout(500); // return prematurely from any operation that takes longer than 500ms

function writeData() {
    const data = Buffer...

    // blocks until write has finished or timeout expires. 
    // If timeout expires, will throw in format "Timeout writing to port: %d of %d bytes written"
    await device.write(data); 
}

function waitForKnownDataSize() {
    
    // blocks until the number of bytes specified are read or the timeout expires.
    // If timeout expires, will return what ever data has been read. 
    // Will not throw if timeout expires
    const data = await device.read(256); 
    if(data.length !== 256) throw new Error("missing data");
}


function pollForAnyData() {
    device.setTimeout(10);
    
    while(true) {
        const data = await device.read(256);

        if(data.length > 0) {
            device.setTimeout(500);
            return data;
        }
    }
}
```

## Credits

This package would not be possible without the folks over at [node-serialport](https://github.com/serialport/node-serialport). This started out
as a fork of their package and has morphed into something new.

## Known Issues

### Electron on Windows: Some actions fail within the first 5 seconds. 

Reloading the page via `location.reload()` or `location = "/"` fixes this issue.

### Electron: Error: Could not locate the bindings file. Tried:...

Ensure you have `electron-builder` setup correctly.

e.g. package.json
```
{
     "scripts": {
        "postinstall": "electron-builder install-app-deps",
     }
}
```