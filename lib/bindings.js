
// use "bindings" instead of node-gyp-build b/c webpack bundling issues with electron
const binding = require("bindings")("faster-serial-port.node");

const AbstractBinding = require('@serialport/binding-abstract')
const serialNumParser = require('./win32-sn-parser');

function promisify(fx) {

  return function(a,b,c) {
    
    const args = [...arguments];

    return new Promise((resolve, reject) => {
      args.push((err, obj) => {
        if(err === null) {
          resolve(obj);
          return;
        }

        reject(err);
      });

      fx.call(null, ...args);
    });
  }
}

const asyncList = promisify(binding.list)
const asyncOpen = promisify(binding.open)
const asyncClose = promisify(binding.close)
const asyncRead = promisify(binding.read);
const asyncBufferedRead = promisify(binding.bufferedRead);
const asyncWrite = promisify(binding.write)
const asyncUpdate = promisify(binding.update)
const asyncSet = promisify(binding.set)
const asyncGet = promisify(binding.get)
const asyncGetBaudRate = promisify(binding.getBaudRate)
const asyncDrain = promisify(binding.drain)
const asyncFlush = promisify(binding.flush)
const { wrapWithHiddenComName } = require('./legacy')

/**
 * The Windows binding layer
 */
class WindowsBinding extends AbstractBinding {

  static async list() {

    const ports = await asyncList();

    // Grab the serial number from the pnp id
    return wrapWithHiddenComName(
      ports.map(port => {
        if (port.pnpId && !port.serialNumber) {
          const serialNumber = serialNumParser(port.pnpId)
          if (serialNumber) {
            return {
              ...port,
              serialNumber,
            }
          }
        }
        return port
      })
    )
  }

  static configureLogging(enabled, dir) {
    binding.configureLogging(enabled, dir);
  }

  constructor(opt = {}) {
    super(opt);

    this.timeout = opt.timeout === undefined ? 10000 : opt.timeout;

    this.bindingOptions = { ...opt.bindingOptions }
    this.fd = null
    this.writeOperation = null
  }

  get isOpen() {
    return this.fd !== null
  }

  async open(path, options) {
    await super.open(path, options)
    this.openOptions = { ...this.bindingOptions, ...options }
    const fd = await asyncOpen(path, this.openOptions)
    this.fd = fd
  }

  async close() {
    await super.close()
    const fd = this.fd
    this.fd = null
    return asyncClose(fd)
  }

  async read(buffer, offset, length) {
    await super.read(buffer, offset, length);
    try {
      const bytesRead = await asyncRead(this.fd, buffer, offset, length, this.timeout);
      return buffer.slice(0, bytesRead);
    } catch (err) {
      if (!this.isOpen) {
        err.canceled = true
      }

      throw err
    }
  }

  async bufferedRead(callback, timeout) {
    await asyncBufferedRead(this.fd, timeout, callback);
  }

  async write(buffer, echoMode) {
    this.writeOperation = super.write(buffer).then(async () => {
      if (buffer.length === 0) {
        return
      }

      await asyncWrite(this.fd, buffer, this.timeout, echoMode);
    });

    return this.writeOperation;
  }

  async update(options) {
    await super.update(options)
    return asyncUpdate(this.fd, options)
  }

  async set(options) {
    await super.set(options)
    return asyncSet(this.fd, options)
  }

  async get() {
    await super.get()
    return asyncGet(this.fd)
  }

  async getBaudRate() {
    await super.get()
    return asyncGetBaudRate(this.fd)
  }

  async drain() {
    await super.drain()
    await this.writeOperation
    return asyncDrain(this.fd)
  }

  async flush() {
    await super.flush()
    return asyncFlush(this.fd)
  }
}

module.exports = WindowsBinding
