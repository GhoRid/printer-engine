#pragma once

#include "printer_factory.h"

#include <string_view>

class SerialPort;

PrinterType detectPrinterType(SerialPort& serialPort);
PrinterType detectPrinterTypeFromResponse(std::string_view response);
