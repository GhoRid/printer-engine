#pragma once

#include <string>

struct ReceiptPrintData {
    std::string name;
    std::string offeringType;
    int amount = 0;
};

bool printReceipt(const ReceiptPrintData& data);