"use strict";

const unsupported = () => {
  throw new Error(
    `printer-engine native printing is not supported on ${process.platform}-${process.arch}.`,
  );
};

const unsupportedModule = {
  isSupported: () => false,
  initialize: unsupported,
  printTest: unsupported,
  printJson: unsupported,
  setForms: unsupported,
  print: unsupported,
  getPrinterType: unsupported,
  getComPorts: unsupported,
  shutdown() {},
};

if (process.platform !== "win32") {
  module.exports = unsupportedModule;
} else if (!["ia32", "x64"].includes(process.arch)) {
  module.exports = unsupportedModule;
} else {
  const native = require(`../prebuilds/win32-${process.arch}/printer_engine_node.node`);

  native.isSupported = () => true;

  module.exports = native;
}
