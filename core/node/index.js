'use strict'

const unsupported = () => {
  throw new Error('printer-engine native printing is currently supported only on Windows.')
}

if (process.platform !== 'win32') {
  module.exports = {
    isSupported: () => false,
    initialize: unsupported,
    printTest: unsupported,
    printJson: unsupported,
    setForms: unsupported,
    print: unsupported,
    getPrinterType: unsupported,
    shutdown() {}
  }
} else {
  if (!['ia32', 'x64'].includes(process.arch)) {
    throw new Error(`printer-engine: Unsupported Windows architecture: ${process.arch}`)
  }

  const native = require(`../prebuilds/win32-${process.arch}/printer_engine_node.node`)
  native.isSupported = () => true
  module.exports = native
}
