#include "access_pass_print.h"

#include "../printer_engine_internal.h"
#include "printer/printer_backend.h"

bool printAccessPass(PE_Printer* printerHandle, const AccessPassPrintData& data)
{
    PrinterBackend* printer = pe_backend(printerHandle);

    if (!printer) {
        return false;
    }

    if (!printer->initialize() ||
        !printer->alignCenter() ||
        !printer->printText("출입증") ||
        !printer->lineFeed(2) ||
        !printer->printText(data.name) ||
        !printer->lineFeed()) {
        return false;
    }

    if (!data.department.empty() &&
        (!printer->printText(data.department) || !printer->lineFeed())) {
        return false;
    }

    return printer->lineFeed() &&
        printer->printQr(data.qrValue) &&
        printer->lineFeed(3) &&
        printer->cut();
}
