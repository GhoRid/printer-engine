#pragma once

#include <string>

struct PE_Printer;

struct ReceiptPrintData {
    std::string name;
    std::string offeringType;
    int amount = 0;
};

bool printReceipt(PE_Printer* printer, const ReceiptPrintData& data);
