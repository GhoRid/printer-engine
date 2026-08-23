#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include <windows.h>
#include <shellapi.h>

#include <mutex>

#include "app_settings.h"
#include "local_server.h"
#include "printer_engine.h"

class AppController
{
public:
    AppController();
    ~AppController();

    bool initialize(HINSTANCE hInstance);
    void shutdown();

private:
    static constexpr UINT WM_TRAY_ICON = WM_APP + 1;

    static constexpr UINT ID_TRAY_OPEN = 1001;
    static constexpr UINT ID_TRAY_EXIT = 1002;
    static constexpr UINT ID_SAVE_SETTINGS = 2001;
    static constexpr UINT ID_TOGGLE_SERVER = 2002;
    static constexpr UINT ID_TEST_PRINT = 2003;

    HINSTANCE hInstance_ = nullptr;
    HWND window_ = nullptr;

    HWND printerTypeCombo_ = nullptr;
    HWND portCombo_ = nullptr;
    HWND baudRateCombo_ = nullptr;
    HWND dataBitsCombo_ = nullptr;
    HWND stopBitsCombo_ = nullptr;
    HWND parityCombo_ = nullptr;
    HWND dpiCombo_ = nullptr;
    HWND printWidthCombo_ = nullptr;
    HWND serverPortCombo_ = nullptr;
    HWND serverButton_ = nullptr;
    HWND statusLabel_ = nullptr;
    HWND logEdit_ = nullptr;

    AppSettings settings_;
    PE_Printer* printer_ = nullptr;
    std::mutex printerMutex_;
    LocalServer localServer_;

    NOTIFYICONDATAW trayIcon_{};

    bool initialized_ = false;
    bool printerReady_ = false;
    bool trayInitialized_ = false;
    bool shuttingDown_ = false;

    bool initializeSettings();
    bool initializePrinter();
    bool initializeWindow();
    bool initializeTray();
    void initializeLocalServer();
    void toggleLocalServer();

    void createWindowControls();
    void loadSettingsToControls();
    bool saveSettingsFromControls(bool notify = true);
    bool applySettings();
    void testPrint();
    void appendLog(const wchar_t* message);
    void updateStatusLabel();

    void showWindow();
    void hideWindow();

    void showTrayMenu();
    void removeTrayIcon();
    void requestExit();

    LRESULT handleWindowMessage(
        HWND hwnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam
    );

    static LRESULT CALLBACK WindowProc(
        HWND hwnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam
    );
};

#endif
