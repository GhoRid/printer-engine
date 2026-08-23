#pragma once

#include <cstdint>
#include <string>
#include <vector>

class SerialPort;

class PrinterBackend
{
public:
    explicit PrinterBackend(SerialPort& serialPort);
    virtual ~PrinterBackend() = default;

    virtual const char* name() const = 0;

    virtual bool initialize();
    virtual bool printText(const std::string& text);
    virtual bool printQr(const std::string& value, int moduleSize = 8);
    virtual bool lineFeed(int lines = 1);
    virtual bool alignLeft();
    virtual bool alignCenter();
    virtual bool alignRight();
    virtual bool cut();

protected:
    SerialPort& serialPort_;

    bool send(const std::vector<std::uint8_t>& data);
    bool send(const std::string& data);
};
