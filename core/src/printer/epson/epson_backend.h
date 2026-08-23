#pragma once

#include "printer/printer_backend.h"

class EpsonBackend final : public PrinterBackend
{
public:
    using PrinterBackend::PrinterBackend;

    const char* name() const override;
};
