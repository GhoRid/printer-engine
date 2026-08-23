#include "printer_detect.h"

#include "serial_port.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>

PrinterType detectPrinterTypeFromResponse(std::string_view response)
{
    std::string uppercase(response);
    std::transform(
        uppercase.begin(),
        uppercase.end(),
        uppercase.begin(),
        [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); }
    );

    if (uppercase.find("EPSON") != std::string::npos) {
        return PrinterType::Epson;
    }

    return PrinterType::Bixolon;
}

PrinterType detectPrinterType(SerialPort& serialPort)
{
    // GS I 66: 제조사 이름 요청. EPSON은 "_EPSON\0" 형태로 응답한다.
    constexpr std::array<unsigned char, 3> makerNameCommand{0x1D, 0x49, 0x42};

    serialPort.discardInput();

    if (!serialPort.write(makerNameCommand.data(), makerNameCommand.size())) {
        return PrinterType::Bixolon;
    }

    std::array<char, 82> buffer{};
    std::size_t received = 0;

    while (received < buffer.size()) {
        const std::size_t chunk = serialPort.read(
            buffer.data() + received,
            buffer.size() - received
        );

        if (chunk == 0) break;
        received += chunk;

        if (std::find(buffer.begin(), buffer.begin() + received, '\0') !=
            buffer.begin() + received) {
            break;
        }
    }

    // 일부 BIXOLON 모델은 제조사 문자열 질의를 지원하지 않는다.
    // ponytail: 응답이 없으면 기존 호환 동작인 BIXOLON으로 폴백한다.
    return detectPrinterTypeFromResponse(
        std::string_view(buffer.data(), received)
    );
}
