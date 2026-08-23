#pragma once

#include <string>

struct PE_Printer;

struct PrintRouteResult {
    int statusCode;
    std::string body;
};

PrintRouteResult routePrintRequest(
    const std::string& path,
    const std::string& body,
    PE_Printer* printer
);
