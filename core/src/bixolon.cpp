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