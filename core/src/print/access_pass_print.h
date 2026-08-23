#pragma once

#include <string>

struct PE_Printer;

struct AccessPassPrintData {
    std::string name;
    std::string department;
    std::string qrValue;
};

bool printAccessPass(PE_Printer* printer, const AccessPassPrintData& data);
