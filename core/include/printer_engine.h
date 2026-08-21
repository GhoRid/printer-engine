#ifndef PRINTER_ENGINE_H
#define PRINTER_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PE_Printer PE_Printer;

typedef enum {
    PE_OK = 0,
    PE_ERROR_INVALID_ARGUMENT,
    PE_ERROR_NOT_INITIALIZED,
    PE_ERROR_CONNECTION,
    PE_ERROR_PRINT
} PE_Result;  //PE_Result 의 결과가 PE_OK 인 경우 정상, 그 외의 경우 에러로 판단

typedef struct {
    const char* printer_type;
    const char* port;

    int baud_rate;
    int data_bits;
    int stop_bits;
    int parity;

    int dpi;
    int print_width_dots;
} PE_PrinterConfig;

PE_Printer* pe_create(void);

PE_Result pe_initialize(
    PE_Printer* printer,
    const PE_PrinterConfig* config
);

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