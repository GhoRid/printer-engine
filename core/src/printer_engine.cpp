#include "printer_engine.h"

#include <iostream>
#include <string>

struct PE_Printer {
    std::string printerType;  //c++은  string타입 없음. std = C++ 표준 라이브러리(ex. stdio), string = 그 안에 있는 문자열 타입
    std::string port;

    int baudRate = 0;        // 통신 속도 (초당 전송 비트 수, 예: 9600, 115200)
    int dataBits = 0;        // 한 번에 전송하는 데이터 비트 수 (보통 8)
    int stopBits = 0;        // 데이터 전송 종료를 알리는 비트 수 (보통 1)
    int parity = 0;          // 통신 오류 검사용 패리티 설정 (없음/홀수/짝수 등)

    int dpi = 0;             // 프린터 해상도 (1인치당 점의 개수, 예: 203dpi)
    int printWidthDots = 0;  // 실제 인쇄 가능한 가로 폭을 dot 단위로 저장

    bool initialized = false; // 프린터 초기화 완료 여부
};

PE_Printer* pe_create(void)
{
    return new PE_Printer();
}

PE_Result pe_initialize(
    PE_Printer* printer,
    const PE_PrinterConfig* config
)
{
    if (!printer || !config) {
        return PE_ERROR_INVALID_ARGUMENT;
    }

    printer->printerType =
        config->printer_type
            ? config->printer_type
            : "";            

    printer->port =
        config->port
            ? config->port
            : "";

    printer->baudRate = config->baud_rate;
    printer->dataBits = config->data_bits;
    printer->stopBits = config->stop_bits;
    printer->parity = config->parity;

    printer->dpi = config->dpi;
    printer->printWidthDots =
        config->print_width_dots;

    printer->initialized = true;

    std::cout  // std:cout = console.log (cout = character output. 문자 출력)
        << "Printer initialized\n"
        << "type: " << printer->printerType << '\n'
        << "port: " << printer->port << '\n'
        << "baud: " << printer->baudRate << '\n'
        << "dpi: " << printer->dpi << '\n'
        << "width: " << printer->printWidthDots << '\n';

    return PE_OK;
}

void pe_shutdown(
    PE_Printer* printer
)
{
    if (!printer) {
        return;
    }

    printer->initialized = false;

    std::cout << "Printer shutdown\n";
}

void pe_destroy(
    PE_Printer* printer
)
{
    delete printer;
}