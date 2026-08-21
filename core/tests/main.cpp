#include "printer_engine.h"

#include <iostream>

int main()
{
    PE_Printer* printer = pe_create();

    PE_PrinterConfig config{
        .printer_type = "BIXOLON",
        .port = "/dev/tty.usbserial",
        .baud_rate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = 0,
        .dpi = 203,
        .print_width_dots = 576,
    };

    PE_Result result =
        pe_initialize(
            printer,
            &config
        );

    if (result != PE_OK) {
        std::cerr
            << "Initialize failed\n";

        pe_destroy(printer);
        return 1;
    }

    std::cout
        << "Printer engine test success\n";

    pe_shutdown(printer);
    pe_destroy(printer);

    return 0;
}