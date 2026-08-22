#include "printer_engine.h"
#include "serial_port.h"

#include <iostream>

int main()
{
    /*
     * 1. 현재 연결된 시리얼 포트 목록 확인
     */
    std::vector<std::string> ports =
        SerialPort::listPorts();

    std::cout
        << "=== Serial Ports ===\n";

    if (ports.empty()) {
        std::cout
            << "No serial ports found\n";
    } else {
        for (const auto& port : ports) {
            std::cout
                << port
                << '\n';
        }
    }

    std::cout
        << "====================\n\n";


    /*
     * 2. 기존 Printer Engine 테스트
     */
    PE_Printer* printer = pe_create();

    PE_PrinterConfig config{
        .printer_type = "BIXOLON",

        // 위에서 출력된 실제 프린터 포트로 변경
        .port = "/dev/tty.usbserial",

        .baud_rate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = 0,

        .dpi = 203,
        .print_width_dots = 576,
    };

    PE_Result result =
        pe_initialize(
            printer,
            &config
        );

    if (result != PE_OK) {
        std::cerr  //에러 출력(character error)
            << "Initialize failed\n";

        pe_destroy(printer);
        return 1;
    }

    std::cout
        << "Printer engine test success\n";

    pe_shutdown(printer);
    pe_destroy(printer);

    return 0;
}