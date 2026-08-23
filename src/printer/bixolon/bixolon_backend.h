#pragma once

#include "printer/printer_backend.h"

class BixolonBackend final : public PrinterBackend
{
public:
    using PrinterBackend::PrinterBackend;

    const char* name() const override;
};
