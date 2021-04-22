const AbstractBinding = require('@serialport/binding-abstract')

class MockBinding extends AbstractBinding {
  static list() {
    return Promise.resolve([])
  }

  constructor(opt = {}) {
    super(opt)
    this.bindingOptions = { ...defaultBindingOptions, ...opt.bindingOptions }
    this.fd = null
    this.writeOperation = null
  }

  get isOpen() {
    return false;
  }

  open(path, options) {
    return Promise.reject("this is a mock")
  }

  close() {
    return Promise.reject("this is a mock")
  }

  read(buffer, offset, length) {
    return Promise.reject("this is a mock")
  }

  write(buffer) {
    return Promise.reject("this is a mock")
  }

  update(options) {
    return Promise.reject("this is a mock")
  }

  set(options) {
    return Promise.reject("this is a mock")
  }

  get() {
    return Promise.reject("this is a mock")
  }

  getBaudRate() {
    return Promise.reject("this is a mock")
  }

  drain() {
    return Promise.reject("this is a mock")
  }

  flush() {
    return Promise.reject("this is a mock")
  }
}

module.exports = MockBinding