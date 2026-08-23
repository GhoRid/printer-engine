// app/settings/app_settings.h

#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include <QString>

struct AppSettings
{
    // 프린터 설정 기본값
    QString printerType = "BIXOLON";
    QString printerPort = "";
    int baudRate = 115200;

    int dataBits = 8;
    int stopBits = 1;
    int parity = 0;

    int dpi = 203;
    int printWidthDots = 576;

    // 로컬 서버 설정 기본값
    QString serverHost = "127.0.0.1";
    int serverPort = 17831;

    // 자동 실행 설정
    bool autoConnectPrinter = false;
    bool autoStartServer = true;
};

#endif