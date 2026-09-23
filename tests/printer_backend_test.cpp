#include "printer/printer_factory.h"
#include "printer/printer_backend.h"
#include "printer/printer_detect.h"
#include "serial_port.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace {
std::vector<std::uint8_t> sent;
std::string response;
std::string makerResponse("_BIXOLON\0", 9);
std::string modelResponse("_BK3-31\0", 8);
}

SerialPort::SerialPort() {}
SerialPort::~SerialPort() {}
bool SerialPort::isOpen() const { return true; }
bool SerialPort::write(const void* data, std::size_t size)
{
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    sent.insert(sent.end(), bytes, bytes + size);
    if (size == 3 && bytes[0] == 0x1D && bytes[1] == 0x49) {
        if (bytes[2] == 0x42) response = makerResponse;
        if (bytes[2] == 0x43) response = modelResponse;
    }
    return true;
}
std::size_t SerialPort::read(void* data, std::size_t size)
{
    const std::size_t count = std::min(size, response.size());
    std::memcpy(data, response.data(), count);
    response.clear();
    return count;
}
void SerialPort::discardInput() { response.clear(); }

int main()
{
    SerialPort serialPort;
    assert(parsePrinterType("BIXOLON_BK") == PrinterType::BixolonBk);
    assert(parsePrinterType("BK3-31") == PrinterType::BixolonBk);
    assert(detectPrinterType(serialPort) == PrinterType::BixolonBk);
    modelResponse = std::string("_SRP-350\0", 9);
    assert(detectPrinterType(serialPort) == PrinterType::Bixolon);
    modelResponse.clear();
    assert(detectPrinterType(serialPort) == PrinterType::Bixolon);
    sent.clear();
    makerResponse = std::string("_EPSON\0", 7);
    assert(detectPrinterType(serialPort) == PrinterType::Epson);
    assert((sent == std::vector<std::uint8_t>{0x1D, 0x49, 0x42}));
    sent.clear();

    auto bixolon = createPrinterBackend(PrinterType::Bixolon, serialPort, 203);
    assert(bixolon->initialize());
    assert((sent == std::vector<std::uint8_t>{0x1B, 0x40, 0x1D, 0x50, 0xC8, 0xC8}));

    sent.clear();
    auto srp = createPrinterBackend(PrinterType::BixolonSrp, serialPort, 203);
    assert(std::string(srp->name()) == "BIXOLON_SRP");
    assert(srp->initialize());
    assert((sent == std::vector<std::uint8_t>{0x1B, 0x40, 0x1D, 0x50, 0xC8, 0xC8}));

    sent.clear();
    auto epson = createPrinterBackend(PrinterType::Epson, serialPort, 203);
    assert(epson->initialize());
    assert((sent == std::vector<std::uint8_t>{0x1B, 0x40, 0x1D, 0x50, 0xC8, 0xC8}));

    sent.clear();
    auto bk = createPrinterBackend(PrinterType::BixolonBk, serialPort, 203);
    assert(std::string(bk->name()) == "BIXOLON_BK");
    assert(bk->initialize());
    assert((sent == std::vector<std::uint8_t>{0x1B, 0x40}));
    assert(bk->setAbsolutePosition(203));
    assert((sent == std::vector<std::uint8_t>{0x1B, 0x40, 0x1B, 0x24, 0xCB, 0x00}));

    sent.clear();
    auto bkWithOtherDpi = createPrinterBackend(PrinterType::BixolonBk, serialPort, 180);
    assert(bkWithOtherDpi->setAbsolutePosition(180));
    assert((sent == std::vector<std::uint8_t>{0x1B, 0x24, 0xCB, 0x00}));
}
