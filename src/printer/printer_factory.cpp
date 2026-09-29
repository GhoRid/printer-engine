#include "printer_factory.h"

#include "bixolon/bixolon_backend.h"
#include "epson/epson_backend.h"

#include <new>

namespace {
class Rx831Backend final : public PrinterBackend {
public:
    using PrinterBackend::PrinterBackend;
    const char* name() const override { return "RX831"; }
    bool printQr(const std::string& value, int moduleSize = 8) override
    {
        if (value.empty() || value.size() > 230 || moduleSize < 1 || moduleSize > 18)
            return false;
        std::vector<std::uint8_t> command{
            0x1D, 0x6C, 0, 0, 0, static_cast<std::uint8_t>(moduleSize),
            static_cast<std::uint8_t>(value.size() & 0xFF),
            static_cast<std::uint8_t>(value.size() >> 8)
        };
        command.insert(command.end(), value.begin(), value.end());
        return send(command);
    }
};
}

std::optional<PrinterType> parsePrinterType(std::string_view value)
{
    if (value == "AUTO") return PrinterType::Auto;
    if (value == "BIXOLON") return PrinterType::Bixolon;
    if (value == "BIXOLON_SRP") return PrinterType::BixolonSrp;
    if (value == "BIXOLON_BK" || value == "BK3-31") return PrinterType::BixolonBk;
    if (value == "EPSON") return PrinterType::Epson;
    if (value == "RX831") return PrinterType::Rx831;
    return std::nullopt;
}

std::unique_ptr<PrinterBackend> createPrinterBackend(
    PrinterType type,
    SerialPort& serialPort,
    int dpi
)
{
    if (type == PrinterType::Epson) {
        return std::unique_ptr<PrinterBackend>(
            new (std::nothrow) EpsonBackend(serialPort, dpi)
        );
    }

    if (type == PrinterType::Rx831) {
        return std::unique_ptr<PrinterBackend>(
            new (std::nothrow) Rx831Backend(serialPort, dpi, true)
        );
    }

    return std::unique_ptr<PrinterBackend>(
        new (std::nothrow) BixolonBackend(
            serialPort, dpi, type == PrinterType::BixolonBk
        )
    );
}
