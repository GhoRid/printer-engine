#include "printer_engine.h"
#include "printer_engine_internal.h"
#include "printer/printer_backend.h"
#include "printer/printer_detect.h"
#include "printer/printer_factory.h"
#include "print/print_router.h"
#include "serial_port.h"

#include <iostream>
#include <memory>
#include <new>
#include <sstream>
#include <string>

struct PE_Printer {
    std::string printerType; // std = C++ 표준 라이브러리 namespace, string = 문자열 클래스
    std::string port;        // 시리얼 포트 이름 (예: COM3)

    int baudRate = 0;        // 통신 속도 (예: 9600, 115200)
    int dataBits = 0;        // 데이터 비트 수 (보통 8)
    int stopBits = 0;        // 정지 비트 수 (보통 1)
    int parity = 0;          // 패리티 설정 (0: 없음, 1: 홀수, 2: 짝수 등)

    int dpi = 0;             // 프린터 해상도 (예: 203dpi)
    int printWidthDots = 0;  // 실제 인쇄 가능한 가로 폭(dot)

    SerialPort serialPort;   // 실제 COM 포트 연결 담당
    std::unique_ptr<PrinterBackend> backend;

    bool initialized = false; // 프린터 초기화 완료 여부
};

namespace {

// 설정값이 정상적인지 검사
bool isValidConfig(const PE_PrinterConfig* config)
{
    if (!config) {
        return false;
    }

    if (!config->printer_type || !parsePrinterType(config->printer_type)) {
        return false;
    }

    // 프린터 출력 정보는 필수
    if (config->dpi <= 0 || config->print_width_dots <= 0) {
        return false;
    }

    // port가 있는 경우에만 시리얼 통신 설정 검사
    if (config->port && config->port[0] != '\0') {
        if (config->baud_rate <= 0) {
            return false;
        }

        if (config->data_bits < 5 || config->data_bits > 8) {
            return false;
        }

        if (config->stop_bits != 1 && config->stop_bits != 2) {
            return false;
        }

        if (config->parity < 0 || config->parity > 4) {
            return false;
        }
    }

    return true;
}

// 개발 빌드에서만 프린터 설정 출력
void printDebugInfo(const PE_Printer* printer)
{
#ifndef NDEBUG
    std::cout
        << "Printer initialized\n"
        << "type: " << printer->printerType << '\n'
        << "port: " << (printer->port.empty() ? "(none)" : printer->port) << '\n'
        << "baud: " << printer->baudRate << '\n'
        << "dpi: " << printer->dpi << '\n'
        << "width: " << printer->printWidthDots << '\n';
#endif
}

} // namespace

PE_Printer* pe_create(void)
{
    // C API 밖으로 bad_alloc 예외가 튀어나가지 않게 처리
    return new (std::nothrow) PE_Printer();
}

PE_Result pe_initialize(
    PE_Printer* printer,
    const PE_PrinterConfig* config
)
{
    if (!printer || !config) {
        return PE_ERROR_INVALID_ARGUMENT;
    }

    if (!isValidConfig(config)) {
        return PE_ERROR_INVALID_ARGUMENT;
    }

    // 이미 초기화되어 있다면 기존 연결부터 종료
    if (printer->initialized) {
        pe_shutdown(printer);
    }

    printer->port = config->port ? config->port : "";

    printer->baudRate = config->baud_rate;
    printer->dataBits = config->data_bits;
    printer->stopBits = config->stop_bits;
    printer->parity = config->parity;

    printer->dpi = config->dpi;
    printer->printWidthDots = config->print_width_dots;

    // COM 포트가 지정되어 있으면 실제 시리얼 포트 연결
    if (!printer->port.empty()) {
        const bool opened = printer->serialPort.open(
            printer->port,
            printer->baudRate,
            printer->dataBits,
            printer->stopBits,
            printer->parity
        );

        if (!opened) {
#ifndef NDEBUG
            std::cerr
                << "Failed to open serial port: "
                << printer->port
                << '\n';
#endif

            printer->initialized = false;

            return PE_ERROR_CONNECTION;
        }
    }

    const PrinterType requestedType = *parsePrinterType(config->printer_type);
    const PrinterType actualType = requestedType == PrinterType::Auto &&
            printer->serialPort.isOpen()
        ? detectPrinterType(printer->serialPort)
        : (requestedType == PrinterType::Auto ? PrinterType::Bixolon : requestedType);

    printer->backend = createPrinterBackend(actualType, printer->serialPort);

    if (!printer->backend) {
        printer->serialPort.close();
        return PE_ERROR_NOT_INITIALIZED;
    }

    printer->printerType = printer->backend->name();

    // port가 없는 경우에는 내장 방식 / 다른 전송 방식으로 사용할 수 있도록
    // 정상 초기화 상태로 처리
    printer->initialized = true;

    printDebugInfo(printer);

    return PE_OK;
}

PE_Result pe_print_test(PE_Printer* printer)
{
    PrinterBackend* backend = pe_backend(printer);

    if (!backend) {
        return PE_ERROR_NOT_INITIALIZED;
    }

    std::ostringstream text;
    text
        << "\x1b@"
        << "Printer Engine Test\n"
        << "------------------------------\n"
        << "Type       : " << printer->printerType << '\n'
        << "Port       : " << printer->port << '\n'
        << "Baud rate  : " << printer->baudRate << '\n'
        << "Data bits  : " << printer->dataBits << '\n'
        << "Stop bits  : " << printer->stopBits << '\n'
        << "Parity     : " << printer->parity << '\n'
        << "DPI        : " << printer->dpi << '\n'
        << "Width(dot) : " << printer->printWidthDots << "\n\n\n";

    return backend->initialize() &&
           backend->printText(text.str()) &&
           backend->lineFeed(3) &&
           backend->cut()
        ? PE_OK
        : PE_ERROR_PRINT;
}

PE_Result pe_print_json(PE_Printer* printer, const char* form, const char* json)
{
    if (!printer || !form || !json) {
        return PE_ERROR_INVALID_ARGUMENT;
    }

    const std::string path = std::string("/print/") + form;
    const PrintRouteResult result = routePrintRequest(path, json, printer);

    if (result.statusCode == 200) return PE_OK;
    if (result.statusCode == 400 || result.statusCode == 404) {
        return PE_ERROR_INVALID_ARGUMENT;
    }
    return result.statusCode == 503
        ? PE_ERROR_PRINT
        : PE_ERROR_NOT_INITIALIZED;
}

PE_Result pe_print_commands(
    PE_Printer* printer,
    const PE_PrintCommand* commands,
    size_t command_count
)
{
    PrinterBackend* backend = pe_backend(printer);
    if (!backend) return PE_ERROR_NOT_INITIALIZED;
    if (!commands || command_count == 0) return PE_ERROR_INVALID_ARGUMENT;
    if (!backend->initialize()) return PE_ERROR_PRINT;

    for (size_t i = 0; i < command_count; ++i) {
        const PE_PrintCommand& command = commands[i];
        bool ok = false;

        switch (command.type) {
            case PE_COMMAND_TEXT:
                if (!command.text) return PE_ERROR_INVALID_ARGUMENT;
                ok = backend->printText(command.text);
                break;
            case PE_COMMAND_ALIGN_LEFT: ok = backend->alignLeft(); break;
            case PE_COMMAND_ALIGN_CENTER: ok = backend->alignCenter(); break;
            case PE_COMMAND_ALIGN_RIGHT: ok = backend->alignRight(); break;
            case PE_COMMAND_FEED:
                if (command.value < 1) return PE_ERROR_INVALID_ARGUMENT;
                ok = backend->lineFeed(command.value);
                break;
            case PE_COMMAND_QR:
                if (!command.text || command.text[0] == '\0') {
                    return PE_ERROR_INVALID_ARGUMENT;
                }
                ok = backend->printQr(command.text);
                break;
            case PE_COMMAND_CUT: ok = backend->cut(); break;
            default: return PE_ERROR_INVALID_ARGUMENT;
        }

        if (!ok) return PE_ERROR_PRINT;
    }

    return PE_OK;
}

const char* pe_get_printer_type(const PE_Printer* printer)
{
    return printer && printer->initialized
        ? printer->printerType.c_str()
        : nullptr;
}

void pe_shutdown(PE_Printer* printer)
{
    if (!printer) {
        return;
    }

    if (!printer->initialized) {
        return;
    }

    // 시리얼 포트를 사용하는 프린터라면 실제 COM 연결 종료
    if (!printer->port.empty()) {
        printer->serialPort.close();
    }

    printer->backend.reset();
    printer->initialized = false;

#ifndef NDEBUG
    std::cout << "Printer shutdown\n";
#endif
}

void pe_destroy(PE_Printer* printer)
{
    if (!printer) {
        return;
    }

    // shutdown을 직접 호출하지 않았어도 안전하게 정리
    pe_shutdown(printer);

    delete printer;
}

PrinterBackend* pe_backend(PE_Printer* printer)
{
    if (!printer || !printer->initialized ||
        !printer->serialPort.isOpen() || !printer->backend) {
        return nullptr;
    }

    return printer->backend.get();
}

int pe_print_width_dots(const PE_Printer* printer)
{
    return printer ? printer->printWidthDots : 0;
}
