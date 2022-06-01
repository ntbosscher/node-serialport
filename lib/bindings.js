const AbstractBinding = require('@serialport/binding-abstract')
const serialNumParser = require('./win32-sn-parser');

let _binding;
// getBinding imports the .node module lazily
// this allows callers to configure node-bindings if required
function getBinding() {
  if(_binding) return _binding;

  // use "node-bindings" instead of node-gyp-build b/c webpack bundling issues with electron
  _binding = require("node-bindings")("faster-serialport.node");
  return _binding;
}

function promisify(fxLookup) {

  return function(a,b,c) {
    
    const args = [...arguments];
    const fx = fxLookup();

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

const asyncList = promisify(() => getBinding().list)
const asyncOpen = promisify(() => getBinding().open)
const asyncClose = promisify(() => getBinding().close)
const asyncRead = promisify(() => getBinding().read);
const asyncBufferedRead = promisify(() => getBinding().bufferedRead);
const asyncWrite = promisify(() => getBinding().write)
const asyncUpdate = promisify(() => getBinding().update)
const asyncSet = promisify(() => getBinding().set)
const asyncGet = promisify(() => getBinding().get)
const asyncGetBaudRate = promisify(() => getBinding().getBaudRate)
const asyncDrain = promisify(() => getBinding().drain)
const asyncFlush = promisify(() => getBinding().flush)
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
    getBinding().configureLogging(enabled, dir);
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
