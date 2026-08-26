#include "serial_port.h"

#include <iostream>
#include <string>
#include <vector>

int main()
{
    // 현재 Mac에 잡힌 시리얼 포트 목록 가져오기
    std::vector<SerialPortInfo> ports = SerialPort::listPorts();

    if (ports.empty()) {
        std::cerr << "No serial ports found\n";
        return 1;
    }

    std::cout << "Available serial ports:\n";

    for (std::size_t i = 0; i < ports.size(); ++i) {
        std::cout << "[" << i << "] " << ports[i].port
                  << " - " << ports[i].description << '\n';
    }

    // 일단 첫 번째 포트 사용
    const std::string& port = ports[0].port;

    std::cout << "\nOpening port: " << port << '\n';

    SerialPort serialPort;

    bool opened = serialPort.open(
        port,
        115200, // baud rate
        8,      // data bits
        1,      // stop bits
        0       // parity
    );

    if (!opened) {
        std::cerr << "Failed to open serial port\n";
        return 1;
    }

    if (!serialPort.isOpen()) {
        std::cerr << "Serial port is not open\n";
        return 1;
    }

    std::cout << "Serial port opened successfully\n";

    // 실제 write()까지 정상 동작하는지 확인
    const std::string testData = "printer-engine serial test\n";

    if (!serialPort.write(
            testData.data(),
            testData.size()
        )) {
        std::cerr << "Failed to write serial data\n";
        serialPort.close();
        return 1;
    }

    std::cout << "Serial data written successfully\n";

    serialPort.close();

    std::cout << "Serial port closed successfully\n";
    std::cout << "SerialPort test success\n";

    return 0;
}
