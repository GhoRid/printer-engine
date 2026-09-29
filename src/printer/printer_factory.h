#pragma once

#include <memory>
#include <optional>
#include <string_view>

class PrinterBackend;
class SerialPort;

enum class PrinterType {
    Auto,
    Bixolon,
    BixolonSrp,
    BixolonBk,
    Epson,
    Rx831
};

std::optional<PrinterType> parsePrinterType(std::string_view value);

std::unique_ptr<PrinterBackend> createPrinterBackend(
    PrinterType type,
    SerialPort& serialPort,
    int dpi = 203
);
