#include "printer_backend.h"

#include "serial_port.h"

#include <cstddef>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

bool utf8ToCp949(const std::string& input, std::string& output)
{
#ifdef _WIN32
    if (input.empty()) {
        output.clear();
        return true;
    }

    const int wideSize = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, input.data(),
        static_cast<int>(input.size()), nullptr, 0
    );
    if (wideSize <= 0) return false;

    std::wstring wide(static_cast<std::size_t>(wideSize), L'\0');
    if (MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, input.data(),
            static_cast<int>(input.size()), wide.data(), wideSize
        ) != wideSize) return false;

    BOOL usedDefaultCharacter = FALSE;
    const int encodedSize = WideCharToMultiByte(
        949, WC_NO_BEST_FIT_CHARS, wide.data(), wideSize,
        nullptr, 0, nullptr, &usedDefaultCharacter
    );
    if (encodedSize <= 0 || usedDefaultCharacter) return false;

    output.assign(static_cast<std::size_t>(encodedSize), '\0');
    usedDefaultCharacter = FALSE;
    return WideCharToMultiByte(
        949, WC_NO_BEST_FIT_CHARS, wide.data(), wideSize,
        output.data(), encodedSize, nullptr, &usedDefaultCharacter
    ) == encodedSize && !usedDefaultCharacter;
#else
    output = input;
    return true;
#endif
}

} // namespace

PrinterBackend::PrinterBackend(SerialPort& serialPort, int dpi)
    : serialPort_(serialPort), dpi_(dpi > 0 ? dpi : 203)
{
}

bool PrinterBackend::initialize()
{
    // Use 200 horizontal/vertical motion units per inch on every supported DPI.
    return send(std::vector<std::uint8_t>{0x1B, 0x40}) &&
        send(std::vector<std::uint8_t>{0x1D, 0x50, 200, 200});
}

bool PrinterBackend::printText(const std::string& text)
{
    std::string encoded;
    if (!utf8ToCp949(text, encoded)) return false;

    // Enable the printer's multibyte CJK mode before sending CP949 text.
    return send(std::vector<std::uint8_t>{0x1C, 0x26}) && send(encoded);
}

bool PrinterBackend::printQr(const std::string& value, int moduleSize)
{
    if (value.empty() || value.size() > 7089 || moduleSize < 1 || moduleSize > 16) {
        return false;
    }

    if (!send(std::vector<std::uint8_t>{
            0x1D, 0x28, 0x6B, 0x04, 0x00, 0x31, 0x41, 0x32, 0x00
        })) return false;

    if (!send(std::vector<std::uint8_t>{
            0x1D, 0x28, 0x6B, 0x03, 0x00, 0x31, 0x43,
            static_cast<std::uint8_t>(moduleSize)
        })) return false;

    if (!send(std::vector<std::uint8_t>{
            0x1D, 0x28, 0x6B, 0x03, 0x00, 0x31, 0x45, 0x31
        })) return false;

    const std::size_t parameterLength = value.size() + 3;
    std::vector<std::uint8_t> store{
        0x1D, 0x28, 0x6B,
        static_cast<std::uint8_t>(parameterLength & 0xFF),
        static_cast<std::uint8_t>((parameterLength >> 8) & 0xFF),
        0x31, 0x50, 0x30
    };
    store.insert(store.end(), value.begin(), value.end());

    return send(store) && send(std::vector<std::uint8_t>{
        0x1D, 0x28, 0x6B, 0x03, 0x00, 0x31, 0x51, 0x30
    });
}

bool PrinterBackend::printImage(
    const std::uint8_t* data,
    std::size_t size,
    int width,
    int height
)
{
    if (!data || width < 1 || height < 1) return false;

    const std::size_t bytesPerRow = static_cast<std::size_t>((width + 7) / 8);
    if (size != bytesPerRow * static_cast<std::size_t>(height) ||
        bytesPerRow > 0xFFFF || height > 0xFFFF) return false;

    std::vector<std::uint8_t> command{
        0x1D, 0x76, 0x30, 0x00,
        static_cast<std::uint8_t>(bytesPerRow & 0xFF),
        static_cast<std::uint8_t>((bytesPerRow >> 8) & 0xFF),
        static_cast<std::uint8_t>(height & 0xFF),
        static_cast<std::uint8_t>((height >> 8) & 0xFF)
    };
    command.insert(command.end(), data, data + size);
    return send(command);
}

bool PrinterBackend::lineFeed(int lines)
{
    return lines <= 0 || send(std::vector<std::uint8_t>(
        static_cast<std::size_t>(lines),
        0x0A
    ));
}

bool PrinterBackend::alignLeft()
{
    return send(std::vector<std::uint8_t>{0x1B, 0x61, 0x00});
}

bool PrinterBackend::alignCenter()
{
    return send(std::vector<std::uint8_t>{0x1B, 0x61, 0x01});
}

bool PrinterBackend::alignRight()
{
    return send(std::vector<std::uint8_t>{0x1B, 0x61, 0x02});
}

bool PrinterBackend::setAbsolutePosition(int dots)
{
    if (dots < 0) return false;

    constexpr int motionUnitsPerInch = 200;
    const long long scaled =
        (static_cast<long long>(dots) * motionUnitsPerInch + (dpi_ / 2)) / dpi_;
    if (scaled > 0xFFFF) return false;
    const int units = static_cast<int>(scaled);

    return send(std::vector<std::uint8_t>{
        0x1B, 0x24,
        static_cast<std::uint8_t>(units & 0xFF),
        static_cast<std::uint8_t>((units >> 8) & 0xFF)
    });
}

bool PrinterBackend::cut()
{
    return send(std::vector<std::uint8_t>{0x1D, 0x56, 0x00});
}

bool PrinterBackend::send(const std::vector<std::uint8_t>& data)
{
    return data.empty() || (serialPort_.isOpen() &&
        serialPort_.write(data.data(), data.size()));
}

bool PrinterBackend::send(const std::string& data)
{
    return data.empty() || (serialPort_.isOpen() &&
        serialPort_.write(data.data(), data.size()));
}
