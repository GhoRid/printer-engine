'use strict'

const assert = require('assert')
const printer = require('./index')

if (process.platform !== 'win32') {
  assert.strictEqual(printer.isSupported(), false)
  assert.throws(() => printer.initialize({}), /not supported/)
  assert.throws(() => printer.getComPorts(), /not supported/)
  assert.doesNotThrow(() => printer.shutdown())
}
else if (printer.isSupported()) {
  assert.throws(
    () => printer.initialize({
      textWidthColumns: 48,
      paddingRightDots: 24,
    }),
    /textWidthColumns cannot be used with paddingLeftDots or paddingRightDots/,
  )
}
