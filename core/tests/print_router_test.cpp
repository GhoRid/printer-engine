#include "print_router.h"
#include "printer/printer_detect.h"
#include "printer_engine.h"

#include <cassert>
#include <string>
#include <string_view>

int main()
{
    PE_Printer* printer = pe_create();
    PE_PrinterConfig config{};
    config.printer_type = "AUTO";
    config.dpi = 203;
    config.print_width_dots = 576;

    assert(printer);
    config.printer_type = "GENERIC_ESC_POS";
    assert(pe_initialize(printer, &config) == PE_ERROR_INVALID_ARGUMENT);
    config.printer_type = "EPSON";
    assert(pe_initialize(printer, &config) == PE_OK);
    assert(std::string(pe_get_printer_type(printer)) == "EPSON");
    config.printer_type = "AUTO";
    assert(pe_initialize(printer, &config) == PE_OK);
    assert(std::string(pe_get_printer_type(printer)) == "BIXOLON");
    pe_destroy(printer);

    assert(detectPrinterTypeFromResponse(
        std::string_view("_EPSON\0", 7)
    ) == PrinterType::Epson);
    assert(detectPrinterTypeFromResponse("BIXOLON") == PrinterType::Bixolon);

    assert(routePrintRequest(
        "/print/receipt",
        R"({"name":"홍길동","amount":10000})",
        nullptr
    ).statusCode == 503);

    assert(routePrintRequest(
        "/print/access-pass",
        R"({"name":"홍길동","qrValue":"ABC123"})",
        nullptr
    ).statusCode == 503);

    assert(routePrintRequest(
        "/print/receipt",
        R"({"name":"홍길동","amount":0})",
        nullptr
    ).statusCode == 400);

    assert(routePrintRequest(
        "/print/access-pass",
        R"({"name":"홍길동"})",
        nullptr
    ).statusCode == 400);

    assert(routePrintRequest(
        "/print/receipt",
        R"({"name":"홍\n길동","amount":10000})",
        nullptr
    ).statusCode == 400);

    assert(routePrintRequest(
        "/print/unknown",
        "not json",
        nullptr
    ).statusCode == 404);
}
