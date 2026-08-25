#include "receipt_print.h"

#include "../printer_engine_internal.h"
#include "../receipt_engine.h"
#include "printer/printer_backend.h"

#include <algorithm>

bool printReceipt(PE_Printer* printerHandle, const ReceiptPrintData& data)
{
    PrinterBackend* printer = pe_backend(printerHandle);

    if (!printer) {
        return false;
    }

    pe::layout::LayoutConfig layout;
    layout.printWidthDots = pe_print_width_dots(printerHandle);
    layout.paddingLeftDots = pe_padding_left_dots(printerHandle);
    layout.paddingRightDots = pe_padding_right_dots(printerHandle);
    layout.asciiCharWidthDots = pe_ascii_char_width_dots(printerHandle);
    layout.textWidthColumns = std::max(
        1,
        layout.contentWidthDots() / layout.asciiCharWidthDots
    );

    pe::ReceiptData receipt;
    receipt.title = "헌금 영수증";
    receipt.headers.push_back("이름: " + data.name);

    if (!data.offeringType.empty()) {
        receipt.headers.push_back("헌금 종류: " + data.offeringType);
    }

    receipt.items.push_back({"금액", std::to_string(data.amount) + "원"});
    receipt.footers.push_back("감사합니다.");

    return printer->initialize() && pe::ReceiptEngine(*printer, layout).print(receipt);
}
