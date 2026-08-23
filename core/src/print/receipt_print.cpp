#include "receipt_print.h"

#include "../printer_engine_internal.h"
#include "../receipt_engine.h"
#include "bixolon.h"
#include "serial_port.h"

bool printReceipt(PE_Printer* printerHandle, const ReceiptPrintData& data)
{
    SerialPort* serialPort = pe_serial_port(printerHandle);

    if (!serialPort) {
        return false;
    }

    Bixolon printer(*serialPort);
    pe::layout::LayoutConfig layout;
    layout.printWidthDots = pe_print_width_dots(printerHandle);

    pe::ReceiptData receipt;
    receipt.title = "헌금 영수증";
    receipt.headers.push_back("이름: " + data.name);

    if (!data.offeringType.empty()) {
        receipt.headers.push_back("헌금 종류: " + data.offeringType);
    }

    receipt.items.push_back({"금액", std::to_string(data.amount) + "원"});
    receipt.footers.push_back("감사합니다.");

    return printer.initialize() && pe::ReceiptEngine(printer, layout).print(receipt);
}
