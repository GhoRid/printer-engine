#include "app_settings.h"

#include <fstream>
#include <string>

bool AppSettings::load(const std::string& filePath)
{
    std::ifstream file(filePath);

    if (!file.is_open()) {
        return false;
    }

    std::string line;

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        std::size_t separator = line.find('=');

        if (separator == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, separator);
        std::string value = line.substr(separator + 1);

        try {
            if (key == "printerType") {
                printerType = value;
            } else if (key == "port") {
                port = value;
            } else if (key == "baudRate") {
                baudRate = std::stoi(value);
            } else if (key == "dataBits") {
                dataBits = std::stoi(value);
            } else if (key == "stopBits") {
                stopBits = std::stoi(value);
            } else if (key == "parity") {
                parity = std::stoi(value);
            } else if (key == "dpi") {
                dpi = std::stoi(value);
            } else if (key == "printWidthDots") {
                printWidthDots = std::stoi(value);
            } else if (key == "serverPort") {
                serverPort = std::stoi(value);
            }
        } catch (...) {
            return false;
        }
    }

    return true;
}

bool AppSettings::save(const std::string& filePath) const
{
    std::ofstream file(filePath);

    if (!file.is_open()) {
        return false;
    }

    file << "printerType=" << printerType << '\n';
    file << "port=" << port << '\n';

    file << "baudRate=" << baudRate << '\n';
    file << "dataBits=" << dataBits << '\n';
    file << "stopBits=" << stopBits << '\n';
    file << "parity=" << parity << '\n';

    file << "dpi=" << dpi << '\n';
    file << "printWidthDots=" << printWidthDots << '\n';

    file << "serverPort=" << serverPort << '\n';

    return true;
}