#include "printer_backend.h"

#include "serial_port.h"

#include <cstddef>

PrinterBackend::PrinterBackend(SerialPort& serialPort)
    : serialPort_(serialPort)
{
}

bool PrinterBackend::initialize()
{
    return send(std::vector<std::uint8_t>{0x1B, 0x40});
}

bool PrinterBackend::printText(const std::string& text)
{
    return send(text);
}

bool PrinterBackend::printQr(const std::string& value, int moduleSize)
{
    if (value.empty() || value.size() > 7089 || moduleSize < 1 || moduleSize > 16) {
        return false;
    }

    if (!send(std::vector<std::uint8_t>{
            0x1D, 0x28, 0x6B, 0x04, 0x00, 0x31, 0x41, 0x32, 0x00
        })) return false;

    if (!send(std::vector<std::uint8_t>{
            0x1D, 0x28, 0x6B, 0x03, 0x00, 0x31, 0x43,
            static_cast<std::uint8_t>(moduleSize)
        })) return false;

    if (!send(std::vector<std::uint8_t>{
            0x1D, 0x28, 0x6B, 0x03, 0x00, 0x31, 0x45, 0x31
        })) return false;

    const std::size_t parameterLength = value.size() + 3;
    std::vector<std::uint8_t> store{
        0x1D, 0x28, 0x6B,
        static_cast<std::uint8_t>(parameterLength & 0xFF),
        static_cast<std::uint8_t>((parameterLength >> 8) & 0xFF),
        0x31, 0x50, 0x30
    };
    store.insert(store.end(), value.begin(), value.end());

    return send(store) && send(std::vector<std::uint8_t>{
        0x1D, 0x28, 0x6B, 0x03, 0x00, 0x31, 0x51, 0x30
    });
}

bool PrinterBackend::lineFeed(int lines)
{
    return lines <= 0 || send(std::vector<std::uint8_t>(
        static_cast<std::size_t>(lines),
        0x0A
    ));
}

bool PrinterBackend::alignLeft()
{
    return send(std::vector<std::uint8_t>{0x1B, 0x61, 0x00});
}

bool PrinterBackend::alignCenter()
{
    return send(std::vector<std::uint8_t>{0x1B, 0x61, 0x01});
}

bool PrinterBackend::alignRight()
{
    return send(std::vector<std::uint8_t>{0x1B, 0x61, 0x02});
}

bool PrinterBackend::cut()
{
    return send(std::vector<std::uint8_t>{0x1D, 0x56, 0x00});
}

bool PrinterBackend::send(const std::vector<std::uint8_t>& data)
{
    return data.empty() || (serialPort_.isOpen() &&
        serialPort_.write(data.data(), data.size()));
}

bool PrinterBackend::send(const std::string& data)
{
    return data.empty() || (serialPort_.isOpen() &&
        serialPort_.write(data.data(), data.size()));
}
