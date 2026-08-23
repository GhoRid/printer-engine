#include "printer_factory.h"

#include "bixolon/bixolon_backend.h"
#include "epson/epson_backend.h"

#include <new>

std::optional<PrinterType> parsePrinterType(std::string_view value)
{
    if (value == "AUTO") return PrinterType::Auto;
    if (value == "BIXOLON") return PrinterType::Bixolon;
    if (value == "EPSON") return PrinterType::Epson;
    return std::nullopt;
}

std::unique_ptr<PrinterBackend> createPrinterBackend(
    PrinterType type,
    SerialPort& serialPort
)
{
    if (type == PrinterType::Epson) {
        return std::unique_ptr<PrinterBackend>(
            new (std::nothrow) EpsonBackend(serialPort)
        );
    }

    return std::unique_ptr<PrinterBackend>(
        new (std::nothrow) BixolonBackend(serialPort)
    );
}
