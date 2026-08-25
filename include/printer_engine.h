#ifndef PRINTER_ENGINE_H
#define PRINTER_ENGINE_H

#include <stddef.h>

// 사용될 때 C++ 컴파일러인지 확인하고, C++ 컴파일러라면 extern "C"를 사용하여 C 스타일의 링크를 사용하도록 지정
#ifdef __cplusplus
extern "C" {
#endif

typedef struct PE_Printer PE_Printer; // 내부는 볼 수 없으나 구조체 타입을 사용할 수 있도록 forward declaration

typedef enum {
    PE_OK = 0,
    PE_ERROR_INVALID_ARGUMENT,
    PE_ERROR_NOT_INITIALIZED,
    PE_ERROR_CONNECTION,
    PE_ERROR_PRINT
} PE_Result;  //PE_Result 의 결과가 PE_OK 인 경우 정상, 그 외의 경우 에러로 판단

typedef struct {
    const char* printer_type;  // 프린터 종류를 나타내는 문자열
    const char* port;          // 연결할 COM/시리얼 포트 이름 또는 경로

    int baud_rate;             // 통신 속도 (예: 9600, 115200)
    int data_bits;             // 한 번에 전송하는 데이터 비트 수 (보통 8)
    int stop_bits;             // 데이터 전송 종료를 알리는 비트 수 (보통 1)
    int parity;                // 통신 오류 검사용 패리티 설정

    int dpi;                   // 프린터 해상도 (1인치당 dot 개수)
    int print_width_dots;      // 인쇄 가능한 가로 폭을 dot 단위로 저장
    int padding_left_dots;     // 콘텐츠 왼쪽 여백. 0 이하면 기본값 24
    int padding_right_dots;    // 콘텐츠 오른쪽 여백. 0 이하면 기본값 24
    int ascii_char_width_dots; // 영문/숫자 한 글자의 기준 폭. 0 이하면 기본값 12
} PE_PrinterConfig;

typedef enum {
    PE_COMMAND_TEXT = 0,
    PE_COMMAND_ALIGN_LEFT,
    PE_COMMAND_ALIGN_CENTER,
    PE_COMMAND_ALIGN_RIGHT,
    PE_COMMAND_FEED,
    PE_COMMAND_QR,
    PE_COMMAND_IMAGE,
    PE_COMMAND_CUT,
    PE_COMMAND_COLUMNS
} PE_CommandType;

typedef struct {
    PE_CommandType type;
    const char* text;
    int value;
    const unsigned char* data;
    size_t data_size;
    int width;
    int height;
    const char* secondary_text; // PE_COMMAND_COLUMNS의 오른쪽 내용
} PE_PrintCommand;


PE_Printer* pe_create(void);

PE_Result pe_initialize(
    PE_Printer* printer,
    const PE_PrinterConfig* config
);

PE_Result pe_print_test(PE_Printer* printer);

PE_Result pe_print_json(
    PE_Printer* printer,
    const char* form,
    const char* json
);

PE_Result pe_print_commands(
    PE_Printer* printer,
    const PE_PrintCommand* commands,
    size_t command_count
);

const char* pe_get_printer_type(const PE_Printer* printer);

void pe_shutdown(
    PE_Printer* printer
);

void pe_destroy(
    PE_Printer* printer
);

#ifdef __cplusplus
}
#endif

#endif
