#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include <string>

#include "printer_engine.h"
#include "receipt_engine.h"

class AppController {

public:
    AppController();
    ~AppController();

    bool initialize(const PE_PrinterConfig& config);

    bool printReceipt(const ReceiptData& receipt);

    void shutdown();

    bool isInitialized() const;

private:
    PE_Printer* printer_ = nullptr;

    ReceiptEngine receiptEngine_;

    bool initialized_ = false;

};

#endif