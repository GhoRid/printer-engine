#include "bixolon.h"
#include "serial_port.h"

#include <cstddef>
#include <cstdint>
#include <vector>

Bixolon::Bixolon(SerialPort& serialPort)
    : serialPort(serialPort)
{
}

bool Bixolon::initialize()
{
    // ESC @
    // 프린터 상태 초기화
    return send(std::vector<std::uint8_t>{
        0x1B,
        0x40
    });
}

bool Bixolon::printText(const std::string& text)
{
    return send(text);
}

bool Bixolon::printQr(const std::string& value, int moduleSize)
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

bool Bixolon::lineFeed(int lines)
{
    if (lines <= 0) {
        return true;
    }
    // 0x0A = LF(Line Feed)
    // 지정된 줄 수만큼 줄바꿈 명령 생성
    std::vector<std::uint8_t> command(
        static_cast<std::size_t>(lines),
        0x0A
    );
    return send(command);
}

bool Bixolon::alignLeft()
{
    // ESC a 0
    // 왼쪽 정렬
    return send(std::vector<std::uint8_t>{
        0x1B,
        0x61,
        0x00
    });
}

bool Bixolon::alignCenter()
{
    // ESC a 1
    // 가운데 정렬
    return send(std::vector<std::uint8_t>{
        0x1B,
        0x61,
        0x01
    });
}

bool Bixolon::alignRight()
{
    // ESC a 2
    // 오른쪽 정렬
    return send(std::vector<std::uint8_t>{
        0x1B,
        0x61,
        0x02
    });
}

bool Bixolon::cut()
{
    // GS V 0
    // 용지 전체 절단
    return send(std::vector<std::uint8_t>{
        0x1D,
        0x56,
        0x00
    });
}

bool Bixolon::send(const std::vector<std::uint8_t>& data)
{
    if (data.empty()) {
        return true;
    }

    if (!serialPort.isOpen()) {
        return false;
    }

    return serialPort.write(
        data.data(),
        data.size()
    );
}

bool Bixolon::send(const std::string& data)
{
    if (data.empty()) {
        return true;
    }

    if (!serialPort.isOpen()) {
        return false;
    }

    return serialPort.write(
        data.data(),
        data.size()
    );
}
