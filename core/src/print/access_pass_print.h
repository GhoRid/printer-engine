#pragma once

#include <string>

struct AccessPassPrintData {
    std::string name;
    std::string department;
    std::string qrValue;
};

bool printAccessPass(const AccessPassPrintData& data);