/* eslint-disable prettier/prettier */

const Binding = require("./bindings");

//  VALIDATION
const DATABITS = Object.freeze([5, 6, 7, 8])
const STOPBITS = Object.freeze([1, 1.5, 2])
const PARITY = Object.freeze(['none', 'even', 'mark', 'odd', 'space'])
const FLOWCONTROLS = Object.freeze(['xon', 'xoff', 'xany', 'rtscts'])

const defaultSettings = Object.freeze()

// const defaultSetFlags = Object.freeze({
//     brk: false,
//     cts: false,
//     dtr: true,
//     dts: false,
//     rts: true,
// })
class FastSerial {

    settings = {
        autoOpen: true,
        endOnClose: false,
        baudRate: 9600,
        dataBits: 8,
        hupcl: true,
        lock: true,
        parity: 'none',
        rtscts: false,
        stopBits: 1,
        xany: false,
        xoff: false,
        xon: false,
    };

    path = "";
    isOpen = false;

    binding = new Binding({});

    constructor(path, options) {
        const settings = { ...this.settings, ...options }
        this.settings = settings;

        if (!path) {
            throw new TypeError(`"path" is not defined: ${path}`)
        }

        this.path = path;

        if (settings.baudrate) {
            throw new TypeError(`"baudrate" is an unknown option, did you mean "baudRate"?`)
        }

        if (typeof settings.baudRate !== 'number') {
            throw new TypeError(`"baudRate" must be a number: ${settings.baudRate}`)
        }

        if (DATABITS.indexOf(settings.dataBits) === -1) {
            throw new TypeError(`"databits" is invalid: ${settings.dataBits}`)
        }

        if (STOPBITS.indexOf(settings.stopBits) === -1) {
            throw new TypeError(`"stopbits" is invalid: ${settings.stopbits}`)
        }

        if (PARITY.indexOf(settings.parity) === -1) {
            throw new TypeError(`"parity" is invalid: ${settings.parity}`)
        }

        this.binding = new Binding({
            bindingOptions: settings.bindingOptions,
        });
    }

    async open() {
        await this.binding.open(this.path, this.settings);
        this.isOpen = true;
    }

    update(options) {
        this.settings.baudRate = options.baudRate;
        return this.binding.update(this.settings);
    }

    close() {
        this.isOpen = false;
        return this.binding.close();
    }

    read(nBytes) {
        return this.binding.read(Buffer.alloc(nBytes), 0, nBytes);
    }

    write(buf) {
        if(Array.isArray(buf)) {
            buf = Buffer.from(buf);
        }

        console.log("internal-write: ", buf.length);

        return this.binding.write(buf);
    }

    static list() {
        return Binding.list();
    }
}

module.exports = FastSerial