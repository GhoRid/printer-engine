#ifndef BIXOLON_H
#define BIXOLON_H

#include <cstdint>
#include <string>
#include <vector>

class SerialPort;

class Bixolon
{
public:
    explicit Bixolon(SerialPort& serialPort);

    // 프린터 상태 초기화
    bool initialize();

    // 문자열 출력
    bool printText(const std::string& text);

    // 줄바꿈
    bool lineFeed(int lines = 1);

    // 왼쪽 정렬
    bool alignLeft();

    // 가운데 정렬
    bool alignCenter();

    // 오른쪽 정렬
    bool alignRight();

    // 용지 절단
    bool cut();

private:
    // 실제 시리얼 포트
    SerialPort& serialPort;

    // ESC/POS 바이트 명령 전송
    bool send(const std::vector<std::uint8_t>& data);

    // 문자열 전송
    bool send(const std::string& data);
};

#endif