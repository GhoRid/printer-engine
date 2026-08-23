'use strict'

const assert = require('assert')
const printer = require('./index')

if (process.platform !== 'win32') {
  assert.strictEqual(printer.isSupported(), false)
  assert.throws(() => printer.initialize({}), /only on Windows/)
  assert.doesNotThrow(() => printer.shutdown())
}
