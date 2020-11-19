/* eslint-disable prettier/prettier */

const Binding = require("./bindings");

//  VALIDATION
const DATABITS = Object.freeze([5, 6, 7, 8])
const STOPBITS = Object.freeze([1, 1.5, 2])
const PARITY = Object.freeze(['none', 'even', 'mark', 'odd', 'space'])
const FLOWCONTROLS = Object.freeze(['xon', 'xoff', 'xany', 'rtscts'])

const defaultSettings = Object.freeze();

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

    _queue = [];

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

    async runQueue() {
        if(this._queue.length === 0) {
            return;
        }

        const callback = this._queue.shift();

        try {
            await callback();
        } catch (e) {
            // ignore errors here
        }

        this.runQueue();
    }

    queue(callback) {
        return new Promise((resolve, reject) => {
            this._queue.push(() => {
                return callback().then(resolve, reject);
            });

            // only item in the queue
            if(this._queue.length === 1) this.runQueue();
        });
    }

    async open() {
        const tStart = new Date().getTime();
        console.log("open-start");
        await this.binding.open(this.path, this.settings);
        this.isOpen = true;
        console.log("open-finished", new Date().getTime() - tStart);
    }

    update(options) {
        this.settings.baudRate = options.baudRate;
        return this.binding.update(this.settings);
    }

    close() {
        this.isOpen = false;
        return this.binding.close();
    }

    setTimeout(ms) {
        this.binding.timeout = ms;
    }

    async read(nBytes) {
        return this.queue(async () => {
            console.log(new Date().getTime(), "internal-read-start:");
            const i = await this.binding.read(Buffer.alloc(nBytes), 0, nBytes);
            console.log(new Date().getTime(), "internal-read-end:");
            return i;
        });
    }

    async write(buf) {
        return this.queue(async () => {
            if(Array.isArray(buf)) {
                buf = Buffer.from(buf);
            }

            console.log(new Date().getTime(), "internal-write-start: ", buf.length);
            const i = await this.binding.write(buf);
            console.log(new Date().getTime(), "internal-write-done: ", buf.length);

            return i;
        });
    }

    static async list() {
        const tStart = new Date().getTime();
        console.log("list-start");

        const list = await Binding.list();

        console.log("list-end", new Date().getTime() - tStart);
        return list;
    }
}

module.exports = FastSerial