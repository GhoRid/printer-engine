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
    config.padding_left_dots = 24;
    config.padding_right_dots = 24;
    config.ascii_char_width_dots = 12;

    assert(printer);
    config.printer_type = "GENERIC_ESC_POS";
    assert(pe_initialize(printer, &config) == PE_ERROR_INVALID_ARGUMENT);
    config.printer_type = "EPSON";
    assert(pe_initialize(printer, &config) == PE_OK);
    assert(std::string(pe_get_printer_type(printer)) == "EPSON");
    config.printer_type = "AUTO";
    assert(pe_initialize(printer, &config) == PE_OK);
    assert(std::string(pe_get_printer_type(printer)) == "BIXOLON");
    config.text_width_columns = 48;
    assert(pe_initialize(printer, &config) == PE_ERROR_INVALID_ARGUMENT);
    config.padding_left_dots = 0;
    config.padding_right_dots = 0;
    assert(pe_initialize(printer, &config) == PE_OK);
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

    assert(pe_print_json(nullptr, "receipt", "{}") == PE_ERROR_INVALID_ARGUMENT);
    assert(pe_print_commands(nullptr, nullptr, 0) == PE_ERROR_NOT_INITIALIZED);
}
