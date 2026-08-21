#include "printer_engine.h"

#include <iostream>
#include <string>

struct PE_Printer {
    std::string printerType;
    std::string port;

    int baudRate = 0;
    int dataBits = 0;
    int stopBits = 0;
    int parity = 0;

    int dpi = 0;
    int printWidthDots = 0;

    bool initialized = false;
};

PE_Printer* pe_create(void)
{
    return new PE_Printer();
}

PE_Result pe_initialize(
    PE_Printer* printer,
    const PE_PrinterConfig* config
)
{
    if (!printer || !config) {
        return PE_ERROR_INVALID_ARGUMENT;
    }

    printer->printerType =
        config->printer_type
            ? config->printer_type
            : "";

    printer->port =
        config->port
            ? config->port
            : "";

    printer->baudRate = config->baud_rate;
    printer->dataBits = config->data_bits;
    printer->stopBits = config->stop_bits;
    printer->parity = config->parity;

    printer->dpi = config->dpi;
    printer->printWidthDots =
        config->print_width_dots;

    printer->initialized = true;

    std::cout
        << "Printer initialized\n"
        << "type: " << printer->printerType << '\n'
        << "port: " << printer->port << '\n'
        << "baud: " << printer->baudRate << '\n'
        << "dpi: " << printer->dpi << '\n'
        << "width: " << printer->printWidthDots << '\n';

    return PE_OK;
}

void pe_shutdown(
    PE_Printer* printer
)
{
    if (!printer) {
        return;
    }

    printer->initialized = false;

    std::cout << "Printer shutdown\n";
}

void pe_destroy(
    PE_Printer* printer
)
{
    delete printer;
}