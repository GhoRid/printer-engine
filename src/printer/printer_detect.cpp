#include "printer_detect.h"

#include "serial_port.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>

namespace {

std::string queryPrinterInfo(SerialPort& serialPort, unsigned char id)
{
    const std::array<unsigned char, 3> command{0x1D, 0x49, id};
    serialPort.discardInput();
    if (!serialPort.write(command.data(), command.size())) return {};

    std::array<char, 82> buffer{};
    std::size_t received = 0;
    while (received < buffer.size()) {
        const std::size_t chunk = serialPort.read(
            buffer.data() + received, buffer.size() - received
        );
        if (chunk == 0) break;
        received += chunk;
        if (std::find(buffer.begin(), buffer.begin() + received, '\0') !=
            buffer.begin() + received) break;
    }
    return std::string(buffer.data(), received);
}

} // namespace

PrinterType detectPrinterTypeFromResponse(std::string_view response)
{
    std::string uppercase(response);
    std::transform(
        uppercase.begin(),
        uppercase.end(),
        uppercase.begin(),
        [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); }
    );

    if (uppercase.find("BK3-") != std::string::npos ||
        uppercase.find("BK5-3") != std::string::npos) {
        return PrinterType::BixolonBk;
    }

    if (uppercase.find("EPSON") != std::string::npos) {
        return PrinterType::Epson;
    }

    return PrinterType::Bixolon;
}

PrinterType detectPrinterType(SerialPort& serialPort)
{
    // GS I 66: 제조사, GS I 67: 모델명. 응답이 없으면 기존 BIXOLON 동작을 유지한다.
    const std::string maker = queryPrinterInfo(serialPort, 0x42);
    if (detectPrinterTypeFromResponse(maker) == PrinterType::Epson) {
        return PrinterType::Epson;
    }
    if (maker.find("BIXOLON") != std::string::npos) {
        return detectPrinterTypeFromResponse(queryPrinterInfo(serialPort, 0x43));
    }
    return PrinterType::Bixolon;
}
