#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include <string>

struct AppSettings {
    std::string printerType = "AUTO";
    std::string port = "COM1";

    int baudRate = 115200;
    int dataBits = 8;
    int stopBits = 1;
    int parity = 0;

    int dpi = 203;
    int printWidthDots = 576;

    int serverPort = 25000;

    bool load(const std::string& filePath);
    bool save(const std::string& filePath) const;
};

#endif
