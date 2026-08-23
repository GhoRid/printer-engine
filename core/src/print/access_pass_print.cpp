#include "access_pass_print.h"

#include "../printer_engine_internal.h"
#include "bixolon.h"
#include "serial_port.h"

bool printAccessPass(PE_Printer* printerHandle, const AccessPassPrintData& data)
{
    SerialPort* serialPort = pe_serial_port(printerHandle);

    if (!serialPort) {
        return false;
    }

    Bixolon printer(*serialPort);

    if (!printer.initialize() ||
        !printer.alignCenter() ||
        !printer.printText("출입증") ||
        !printer.lineFeed(2) ||
        !printer.printText(data.name) ||
        !printer.lineFeed()) {
        return false;
    }

    if (!data.department.empty() &&
        (!printer.printText(data.department) || !printer.lineFeed())) {
        return false;
    }

    return printer.lineFeed() &&
        printer.printQr(data.qrValue) &&
        printer.lineFeed(3) &&
        printer.cut();
}
