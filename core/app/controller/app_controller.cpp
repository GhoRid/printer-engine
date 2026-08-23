#include "app_controller.h"
#include "print_router.h"
#include "resource.h"
#include "serial_port.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr wchar_t WINDOW_CLASS_NAME[] = L"PrinterEngineWindowClass";
constexpr wchar_t WINDOW_TITLE[] = L"Printer Engine";

// UTF-8 std::string -> Windows UTF-16 std::wstring
std::wstring utf8ToWide(const std::string& value)
{
    if (value.empty()) {
        return L"";
    }

    const int size = MultiByteToWideChar(
        CP_UTF8,
        0,
        value.c_str(),
        static_cast<int>(value.size()),
        nullptr,
        0
    );

    if (size <= 0) {
        return L"";
    }

    std::wstring result(
        static_cast<std::size_t>(size),
        L'\0'
    );

    MultiByteToWideChar(
        CP_UTF8,
        0,
        value.c_str(),
        static_cast<int>(value.size()),
        result.data(),
        size
    );

    return result;
}

// Windows UTF-16 std::wstring -> UTF-8 std::string
std::string wideToUtf8(const std::wstring& value)
{
    if (value.empty()) {
        return "";
    }

    const int size = WideCharToMultiByte(
        CP_UTF8,
        0,
        value.c_str(),
        static_cast<int>(value.size()),
        nullptr,
        0,
        nullptr,
        nullptr
    );

    if (size <= 0) {
        return "";
    }

    std::string result(
        static_cast<std::size_t>(size),
        '\0'
    );

    WideCharToMultiByte(
        CP_UTF8,
        0,
        value.c_str(),
        static_cast<int>(value.size()),
        result.data(),
        size,
        nullptr,
        nullptr
    );

    return result;
}

void setDefaultFont(HWND control)
{
    if (control == nullptr) {
        return;
    }

    SendMessageW(
        control,
        WM_SETFONT,
        reinterpret_cast<WPARAM>(
            GetStockObject(DEFAULT_GUI_FONT)
        ),
        TRUE
    );
}

void addComboItem(HWND combo, const wchar_t* text)
{
    SendMessageW(
        combo,
        CB_ADDSTRING,
        0,
        reinterpret_cast<LPARAM>(text)
    );
}

bool selectComboValue(HWND combo, const std::wstring& value)
{
    const LRESULT index = SendMessageW(
        combo,
        CB_FINDSTRINGEXACT,
        static_cast<WPARAM>(-1),
        reinterpret_cast<LPARAM>(value.c_str())
    );

    if (index == CB_ERR) {
        return false;
    }

    SendMessageW(
        combo,
        CB_SETCURSEL,
        static_cast<WPARAM>(index),
        0
    );

    return true;
}

void selectOrAddComboValue(HWND combo, const std::wstring& value)
{
    if (selectComboValue(combo, value)) {
        return;
    }

    addComboItem(
        combo,
        value.c_str()
    );

    selectComboValue(
        combo,
        value
    );
}

std::wstring getControlText(HWND control)
{
    wchar_t buffer[256]{};

    GetWindowTextW(
        control,
        buffer,
        256
    );

    return buffer;
}

} // namespace

AppController::AppController() = default;

AppController::~AppController()
{
    shutdown();
}

bool AppController::initialize(HINSTANCE hInstance)
{
    if (initialized_) {
        return true;
    }

    hInstance_ = hInstance;

    if (!initializeSettings()) {
        return false;
    }

    // 프린터 연결이 실패해도 설정 GUI는 떠야 하므로 GUI 먼저 생성
    if (!initializeWindow()) {
        shutdown();
        return false;
    }

    if (!initializeTray()) {
        shutdown();
        return false;
    }

    // 프린터 연결 실패는 앱 전체 실행 실패로 처리하지 않음
    printerReady_ = initializePrinter();

    updateStatusLabel();
    appendLog(printerReady_ ? L"프린터 연결 완료." : L"프린터 연결 안 됨.");

    initializeLocalServer();

    initialized_ = true;

    showWindow();

    return true;
}

void AppController::shutdown()
{
    if (shuttingDown_) {
        return;
    }

    shuttingDown_ = true;

    localServer_.stop();

    removeTrayIcon();

    if (printer_ != nullptr) {
        pe_shutdown(printer_);
        pe_destroy(printer_);

        printer_ = nullptr;
    }

    printerReady_ = false;

    if (
        window_ != nullptr &&
        IsWindow(window_)
    ) {
        DestroyWindow(window_);
    }

    window_ = nullptr;
    hInstance_ = nullptr;

    initialized_ = false;
}

bool AppController::initializeSettings()
{
    if (!settings_.load("settings.conf")) {
        if (!settings_.save("settings.conf")) {
            return false;
        }
    }

    return true;
}

bool AppController::initializePrinter()
{
    const std::lock_guard lock(printerMutex_);

    if (printer_ != nullptr) {
        pe_shutdown(printer_);
        pe_destroy(printer_);

        printer_ = nullptr;
    }

    if (settings_.port.empty()) {
        return false;
    }

    printer_ = pe_create();

    if (printer_ == nullptr) {
        return false;
    }

    PE_PrinterConfig config{};

    config.printer_type =
        settings_.printerType.c_str();

    config.port =
        settings_.port.c_str();

    config.baud_rate =
        settings_.baudRate;

    config.data_bits =
        settings_.dataBits;

    config.stop_bits =
        settings_.stopBits;

    config.parity =
        settings_.parity;

    config.dpi =
        settings_.dpi;

    config.print_width_dots =
        settings_.printWidthDots;

    const PE_Result result = pe_initialize(
        printer_,
        &config
    );

    if (result != PE_OK) {
        pe_destroy(printer_);

        printer_ = nullptr;

        return false;
    }

    return true;
}

bool AppController::initializeWindow()
{
    WNDCLASSEXW windowClass{};

    windowClass.cbSize =
        sizeof(WNDCLASSEXW);

    windowClass.lpfnWndProc =
        AppController::WindowProc;

    windowClass.hInstance =
        hInstance_;

    windowClass.hCursor = LoadCursor(
        nullptr,
        IDC_ARROW
    );

    windowClass.hIcon = LoadIconW(
        hInstance_,
        MAKEINTRESOURCEW(IDI_APP_ICON)
    );

    windowClass.hIconSm = LoadIconW(
        hInstance_,
        MAKEINTRESOURCEW(IDI_APP_ICON)
    );

    windowClass.hbrBackground =
        GetSysColorBrush(
            COLOR_BTNFACE
        );

    windowClass.lpszClassName =
        WINDOW_CLASS_NAME;

    if (!RegisterClassExW(&windowClass)) {
        const DWORD error =
            GetLastError();

        if (
            error !=
            ERROR_CLASS_ALREADY_EXISTS
        ) {
            return false;
        }
    }

    window_ = CreateWindowExW(
        0,
        WINDOW_CLASS_NAME,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        900,
        780,
        nullptr,
        nullptr,
        hInstance_,
        this
    );

    if (window_ == nullptr) {
        return false;
    }

    createWindowControls();

    return true;
}

void AppController::createWindowControls()
{
    auto createLabel =
        [this](
            const wchar_t* text,
            int x,
            int y,
            int width
        )
    {
        HWND control = CreateWindowW(
            L"STATIC",
            text,
            WS_VISIBLE | WS_CHILD,
            x,
            y,
            width,
            24,
            window_,
            nullptr,
            hInstance_,
            nullptr
        );

        setDefaultFont(control);

        return control;
    };

    auto createCombo =
        [this](
            int x,
            int y,
            DWORD style = CBS_DROPDOWNLIST
        )
    {
        HWND control = CreateWindowW(
            L"COMBOBOX",
            nullptr,
            WS_VISIBLE |
            WS_CHILD |
            WS_TABSTOP |
            WS_VSCROLL |
            style,
            x,
            y,
            330,
            220,
            window_,
            nullptr,
            hInstance_,
            nullptr
        );

        setDefaultFont(control);

        return control;
    };

    HWND title = CreateWindowW(
        L"STATIC",
        L"Printer Engine 설정",
        WS_VISIBLE | WS_CHILD,
        30,
        25,
        400,
        30,
        window_,
        nullptr,
        hInstance_,
        nullptr
    );

    setDefaultFont(title);

    // 프린터 종류
    createLabel(
        L"프린터 종류",
        30,
        80,
        120
    );

    printerTypeCombo_ =
        createCombo(
            220,
            75
        );

    addComboItem(
        printerTypeCombo_,
        L"BIXOLON"
    );

    // COM 포트
    createLabel(
        L"COM 포트",
        30,
        120,
        120
    );

    portCombo_ =
        createCombo(
            220,
            115
        );

    addComboItem(
        portCombo_,
        L"(사용 안 함)"
    );

    // Windows에서 현재 실제 등록된 COM 포트 조회
    const std::vector<std::string> ports =
        SerialPort::listPorts();

    for (const std::string& port : ports) {
        const std::wstring widePort =
            utf8ToWide(port);

        addComboItem(
            portCombo_,
            widePort.c_str()
        );
    }

    // 통신 속도
    createLabel(
        L"통신 속도",
        30,
        160,
        120
    );

    baudRateCombo_ =
        createCombo(
            220,
            155
        );

    addComboItem(baudRateCombo_, L"9600");
    addComboItem(baudRateCombo_, L"19200");
    addComboItem(baudRateCombo_, L"38400");
    addComboItem(baudRateCombo_, L"57600");
    addComboItem(baudRateCombo_, L"115200");

    // 데이터 비트
    createLabel(
        L"데이터 비트",
        30,
        200,
        120
    );

    dataBitsCombo_ =
        createCombo(
            220,
            195
        );

    addComboItem(dataBitsCombo_, L"5");
    addComboItem(dataBitsCombo_, L"6");
    addComboItem(dataBitsCombo_, L"7");
    addComboItem(dataBitsCombo_, L"8");

    // 정지 비트
    createLabel(
        L"정지 비트",
        30,
        240,
        120
    );

    stopBitsCombo_ =
        createCombo(
            220,
            235
        );

    addComboItem(stopBitsCombo_, L"1");
    addComboItem(stopBitsCombo_, L"2");

    // 패리티
    createLabel(
        L"패리티",
        30,
        280,
        120
    );

    parityCombo_ =
        createCombo(
            220,
            275
        );

    addComboItem(parityCombo_, L"없음");
    addComboItem(parityCombo_, L"홀수");
    addComboItem(parityCombo_, L"짝수");
    addComboItem(parityCombo_, L"Mark");
    addComboItem(parityCombo_, L"Space");

    // DPI
    createLabel(
        L"DPI",
        30,
        320,
        120
    );

    dpiCombo_ =
        createCombo(
            220,
            315
        );

    addComboItem(dpiCombo_, L"203");
    addComboItem(dpiCombo_, L"300");

    // 출력 폭
    createLabel(
        L"출력 폭 (dot)",
        30,
        360,
        120
    );

    printWidthCombo_ =
        createCombo(
            220,
            355
        );

    addComboItem(printWidthCombo_, L"384");
    addComboItem(printWidthCombo_, L"512");
    addComboItem(printWidthCombo_, L"576");

    // 서버 포트
    createLabel(
        L"서버 포트",
        30,
        400,
        120
    );

    // 드롭다운 선택 + 직접 입력 가능
    serverPortCombo_ =
        createCombo(
            220,
            395,
            CBS_DROPDOWN
        );

    addComboItem(
        serverPortCombo_,
        L"18080"
    );

    addComboItem(
        serverPortCombo_,
        L"18081"
    );

    addComboItem(
        serverPortCombo_,
        L"8080"
    );

    HWND testButton = CreateWindowW(
        L"BUTTON",
        L"테스트 출력",
        WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
        625,
        75,
        190,
        45,
        window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_TEST_PRINT)),
        hInstance_,
        nullptr
    );
    setDefaultFont(testButton);

    createLabel(L"로그", 30, 440, 120);
    logEdit_ = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        nullptr,
        WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_MULTILINE |
            ES_AUTOVSCROLL | ES_READONLY,
        30,
        465,
        520,
        150,
        window_,
        nullptr,
        hInstance_,
        nullptr
    );
    setDefaultFont(logEdit_);

    serverButton_ = CreateWindowW(
        L"BUTTON",
        L"서버 열기",
        WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
        625,
        650,
        190,
        45,
        window_,
        reinterpret_cast<HMENU>(
            static_cast<INT_PTR>(ID_TOGGLE_SERVER)
        ),
        hInstance_,
        nullptr
    );

    setDefaultFont(serverButton_);

    // 저장 버튼
    HWND saveButton = CreateWindowW(
        L"BUTTON",
        L"설정 저장",
        WS_VISIBLE |
        WS_CHILD |
        WS_TABSTOP |
        BS_PUSHBUTTON,
        625,
        590,
        190,
        45,
        window_,
        reinterpret_cast<HMENU>(
            static_cast<INT_PTR>(
                ID_SAVE_SETTINGS
            )
        ),
        hInstance_,
        nullptr
    );

    setDefaultFont(saveButton);

    // 프린터 상태
    createLabel(
        L"프린터 상태",
        30,
        630,
        120
    );

    statusLabel_ = CreateWindowW(
        L"STATIC",
        L"",
        WS_VISIBLE | WS_CHILD,
        220,
        630,
        260,
        24,
        window_,
        nullptr,
        hInstance_,
        nullptr
    );

    setDefaultFont(
        statusLabel_
    );

    HWND infoLabel = CreateWindowW(
        L"STATIC",
        L"설정 저장 후 서버를 열면 새 설정이 즉시 적용됩니다.\n"
        L"창을 닫아도 시스템 트레이에서 계속 실행됩니다.",
        WS_VISIBLE | WS_CHILD,
        30,
        680,
        520,
        45,
        window_,
        nullptr,
        hInstance_,
        nullptr
    );

    setDefaultFont(infoLabel);

    loadSettingsToControls();
    appendLog(L"Printer Engine 시작 중...");
    appendLog(settings_.port.empty() ? L"사용 가능한 프린터 포트가 없습니다." : L"설정 파일 로드 완료.");
}

void AppController::loadSettingsToControls()
{
    const std::wstring printerType =
        utf8ToWide(
            settings_.printerType
        );

    selectOrAddComboValue(
        printerTypeCombo_,
        printerType
    );

    // COM 포트
    if (settings_.port.empty()) {
        selectComboValue(
            portCombo_,
            L"(사용 안 함)"
        );
    }
    else {
        const std::wstring port =
            utf8ToWide(settings_.port);

        // 실제 현재 존재하는 COM 포트인 경우에만 선택
        if (!selectComboValue(
            portCombo_,
            port
        )) {
            // 없는 COM 포트를 강제로 목록에 추가하지 않음
            selectComboValue(
                portCombo_,
                L"(사용 안 함)"
            );
        }
    }

    selectOrAddComboValue(
        baudRateCombo_,
        std::to_wstring(
            settings_.baudRate
        )
    );

    selectOrAddComboValue(
        dataBitsCombo_,
        std::to_wstring(
            settings_.dataBits
        )
    );

    selectOrAddComboValue(
        stopBitsCombo_,
        std::to_wstring(
            settings_.stopBits
        )
    );

    int parityIndex =
        settings_.parity;

    if (
        parityIndex < 0 ||
        parityIndex > 4
    ) {
        parityIndex = 0;
    }

    SendMessageW(
        parityCombo_,
        CB_SETCURSEL,
        static_cast<WPARAM>(
            parityIndex
        ),
        0
    );

    selectOrAddComboValue(
        dpiCombo_,
        std::to_wstring(
            settings_.dpi
        )
    );

    selectOrAddComboValue(
        printWidthCombo_,
        std::to_wstring(
            settings_.printWidthDots
        )
    );

    SetWindowTextW(
        serverPortCombo_,
        std::to_wstring(
            settings_.serverPort
        ).c_str()
    );
}

bool AppController::saveSettingsFromControls(bool notify)
{
    const std::wstring printerType =
        getControlText(
            printerTypeCombo_
        );

    const std::wstring port =
        getControlText(
            portCombo_
        );

    const std::wstring baudRate =
        getControlText(
            baudRateCombo_
        );

    const std::wstring dataBits =
        getControlText(
            dataBitsCombo_
        );

    const std::wstring stopBits =
        getControlText(
            stopBitsCombo_
        );

    const std::wstring dpi =
        getControlText(
            dpiCombo_
        );

    const std::wstring printWidth =
        getControlText(
            printWidthCombo_
        );

    const std::wstring serverPort =
        getControlText(
            serverPortCombo_
        );

    if (
        printerType.empty() ||
        baudRate.empty() ||
        dataBits.empty() ||
        stopBits.empty() ||
        dpi.empty() ||
        printWidth.empty() ||
        serverPort.empty()
    ) {
        MessageBoxW(
            window_,
            L"모든 설정값을 입력해 주세요.",
            L"설정 오류",
            MB_OK | MB_ICONWARNING
        );

        return false;
    }

    try {
        const int newBaudRate =
            std::stoi(baudRate);

        const int newDataBits =
            std::stoi(dataBits);

        const int newStopBits =
            std::stoi(stopBits);

        const int newDpi =
            std::stoi(dpi);

        const int newPrintWidth =
            std::stoi(printWidth);

        const int newServerPort =
            std::stoi(serverPort);

        const LRESULT parityIndex =
            SendMessageW(
                parityCombo_,
                CB_GETCURSEL,
                0,
                0
            );

        if (parityIndex == CB_ERR) {
            MessageBoxW(
                window_,
                L"패리티 값을 선택해 주세요.",
                L"설정 오류",
                MB_OK | MB_ICONWARNING
            );

            return false;
        }

        if (
            newServerPort <= 0 ||
            newServerPort > 65535
        ) {
            MessageBoxW(
                window_,
                L"서버 포트는 1~65535 사이여야 합니다.",
                L"설정 오류",
                MB_OK | MB_ICONWARNING
            );

            return false;
        }

        settings_.printerType =
            wideToUtf8(
                printerType
            );

        if (port == L"(사용 안 함)") {
            settings_.port.clear();
        }
        else {
            settings_.port =
                wideToUtf8(
                    port
                );
        }

        settings_.baudRate =
            newBaudRate;

        settings_.dataBits =
            newDataBits;

        settings_.stopBits =
            newStopBits;

        settings_.parity =
            static_cast<int>(
                parityIndex
            );

        settings_.dpi =
            newDpi;

        settings_.printWidthDots =
            newPrintWidth;

        settings_.serverPort =
            newServerPort;
    }
    catch (...) {
        MessageBoxW(
            window_,
            L"설정값 형식이 올바르지 않습니다.",
            L"설정 오류",
            MB_OK | MB_ICONWARNING
        );

        return false;
    }

    if (!settings_.save(
        "settings.conf"
    )) {
        MessageBoxW(
            window_,
            L"settings.conf 파일 저장에 실패했습니다.",
            L"저장 실패",
            MB_OK | MB_ICONERROR
        );

        return false;
    }

    if (notify) {
        MessageBoxW(window_, L"설정이 저장되었습니다.", L"Printer Engine", MB_OK | MB_ICONINFORMATION);
    }

    appendLog(L"설정 저장 완료.");

    return true;
}

bool AppController::applySettings()
{
    if (!saveSettingsFromControls(false)) {
        return false;
    }

    printerReady_ = initializePrinter();
    updateStatusLabel();
    appendLog(printerReady_ ? L"프린터 연결 완료." : L"프린터 연결 실패 또는 포트 없음.");
    return true;
}

void AppController::testPrint()
{
    if (!applySettings() || !printerReady_) {
        MessageBoxW(window_, L"연결된 프린터가 없습니다.", L"테스트 출력", MB_OK | MB_ICONWARNING);
        return;
    }

    const std::lock_guard lock(printerMutex_);
    const bool printed = pe_print_test(printer_) == PE_OK;
    appendLog(printed ? L"설정값 테스트 출력 완료." : L"테스트 출력 실패.");
    MessageBoxW(window_, printed ? L"설정값을 테스트 출력했습니다." : L"테스트 출력에 실패했습니다.",
        L"테스트 출력", MB_OK | (printed ? MB_ICONINFORMATION : MB_ICONERROR));
}

void AppController::appendLog(const wchar_t* message)
{
    if (!logEdit_) return;
    SYSTEMTIME now{};
    GetLocalTime(&now);
    wchar_t line[512]{};
    swprintf_s(line, L"[%04d-%02d-%02d %02d:%02d:%02d] %s\r\n",
        now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond, message);
    SendMessageW(logEdit_, EM_SETSEL, static_cast<WPARAM>(-1), static_cast<LPARAM>(-1));
    SendMessageW(logEdit_, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(line));
}

void AppController::updateStatusLabel()
{
    if (statusLabel_ == nullptr) {
        return;
    }

    SetWindowTextW(
        statusLabel_,
        printerReady_
            ? L"연결됨"
            : L"연결 안 됨"
    );
}

bool AppController::initializeTray()
{
    trayIcon_ = {};

    trayIcon_.cbSize =
        sizeof(NOTIFYICONDATAW);

    trayIcon_.hWnd =
        window_;

    trayIcon_.uID = 1;

    trayIcon_.uFlags =
        NIF_MESSAGE |
        NIF_ICON |
        NIF_TIP;

    trayIcon_.uCallbackMessage =
        WM_TRAY_ICON;

    trayIcon_.hIcon = LoadIconW(
        hInstance_,
        MAKEINTRESOURCEW(
            IDI_APP_ICON
        )
    );

    wcscpy_s(
        trayIcon_.szTip,
        L"Printer Engine"
    );

    if (!Shell_NotifyIconW(
        NIM_ADD,
        &trayIcon_
    )) {
        return false;
    }

    trayIcon_.uVersion =
        NOTIFYICON_VERSION_4;

    Shell_NotifyIconW(
        NIM_SETVERSION,
        &trayIcon_
    );

    trayInitialized_ = true;

    return true;
}

void AppController::initializeLocalServer()
{
    localServer_.setHealthHandler(
        [this]() {
            HttpResponse response;

            response.statusCode = 200;

            response.contentType =
                "application/json";

            if (printerReady_) {
                response.body =
                    R"({"status":"ok","printerInitialized":true})";
            }
            else {
                response.body =
                    R"({"status":"ok","printerInitialized":false})";
            }

            return response;
        }
    );

    localServer_.setPrintHandler(
        [this](const std::string& path, const std::string& body) {
            const std::lock_guard lock(printerMutex_);
            const PrintRouteResult result = routePrintRequest(
                path,
                body,
                printer_
            );

            return HttpResponse{
                result.statusCode,
                "application/json",
                result.body
            };
        }
    );

}

void AppController::toggleLocalServer()
{
    if (localServer_.isRunning()) {
        localServer_.stop();
        SetWindowTextW(serverButton_, L"서버 열기");
        appendLog(L"서버 정지.");
        return;
    }

    try {
        if (!applySettings()) {
            return;
        }

        const int port = std::stoi(
            getControlText(serverPortCombo_)
        );

        if (port <= 0 || port > 65535) {
            throw std::out_of_range("port");
        }

        if (localServer_.start(port)) {
            SetWindowTextW(serverButton_, L"서버 정지");
            appendLog(L"서버 준비 완료.");
            return;
        }
    }
    catch (...) {
    }

    MessageBoxW(
        window_,
        L"로컬 서버를 시작하지 못했습니다.\n포트 번호와 사용 여부를 확인해 주세요.",
        L"Printer Engine",
        MB_OK | MB_ICONERROR
    );
}

void AppController::showWindow()
{
    if (window_ == nullptr) {
        return;
    }

    ShowWindow(
        window_,
        SW_SHOW
    );

    SetForegroundWindow(
        window_
    );
}

void AppController::hideWindow()
{
    if (window_ == nullptr) {
        return;
    }

    ShowWindow(
        window_,
        SW_HIDE
    );
}

void AppController::showTrayMenu()
{
    HMENU menu =
        CreatePopupMenu();

    if (menu == nullptr) {
        return;
    }

    AppendMenuW(
        menu,
        MF_STRING,
        ID_TRAY_OPEN,
        L"열기"
    );

    AppendMenuW(
        menu,
        MF_SEPARATOR,
        0,
        nullptr
    );

    AppendMenuW(
        menu,
        MF_STRING,
        ID_TRAY_EXIT,
        L"종료"
    );

    POINT cursor{};

    GetCursorPos(
        &cursor
    );

    SetForegroundWindow(
        window_
    );

    TrackPopupMenu(
        menu,
        TPM_RIGHTBUTTON |
        TPM_BOTTOMALIGN |
        TPM_LEFTALIGN,
        cursor.x,
        cursor.y,
        0,
        window_,
        nullptr
    );

    DestroyMenu(
        menu
    );
}

void AppController::removeTrayIcon()
{
    if (!trayInitialized_) {
        return;
    }

    Shell_NotifyIconW(
        NIM_DELETE,
        &trayIcon_
    );

    trayInitialized_ = false;
}

void AppController::requestExit()
{
    if (shuttingDown_) {
        return;
    }

    shutdown();

    PostQuitMessage(0);
}

LRESULT AppController::handleWindowMessage(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (message) {
        // STATIC 컨트롤 뒤의 흰 배경 제거
        case WM_CTLCOLORSTATIC:
        {
            HDC hdc =
                reinterpret_cast<HDC>(
                    wParam
                );

            SetBkMode(
                hdc,
                TRANSPARENT
            );

            return reinterpret_cast<LRESULT>(
                GetSysColorBrush(
                    COLOR_BTNFACE
                )
            );
        }

        case WM_CLOSE:
        {
            // X 버튼은 종료가 아니라 숨김
            hideWindow();

            return 0;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) {
                case ID_SAVE_SETTINGS:
                {
                    saveSettingsFromControls();

                    return 0;
                }

                case ID_TEST_PRINT:
                {
                    testPrint();
                    return 0;
                }

                case ID_TOGGLE_SERVER:
                {
                    toggleLocalServer();

                    return 0;
                }

                case ID_TRAY_OPEN:
                {
                    showWindow();

                    return 0;
                }

                case ID_TRAY_EXIT:
                {
                    requestExit();

                    return 0;
                }
            }

            break;
        }

        case WM_TRAY_ICON:
        {
            switch (LOWORD(lParam)) {
                case WM_LBUTTONDBLCLK:
                {
                    showWindow();

                    return 0;
                }

                case WM_CONTEXTMENU:
                case WM_RBUTTONUP:
                {
                    showTrayMenu();

                    return 0;
                }
            }

            break;
        }

        case WM_DESTROY:
        {
            window_ = nullptr;

            if (!shuttingDown_) {
                PostQuitMessage(0);
            }

            return 0;
        }
    }

    return DefWindowProcW(
        hwnd,
        message,
        wParam,
        lParam
    );
}

LRESULT CALLBACK AppController::WindowProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
)
{
    AppController* controller =
        nullptr;

    if (message == WM_NCCREATE) {
        const auto* createStruct =
            reinterpret_cast<CREATESTRUCTW*>(
                lParam
            );

        controller =
            static_cast<AppController*>(
                createStruct->lpCreateParams
            );

        SetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(
                controller
            )
        );
    }
    else {
        controller =
            reinterpret_cast<AppController*>(
                GetWindowLongPtrW(
                    hwnd,
                    GWLP_USERDATA
                )
            );
    }

    if (controller != nullptr) {
        return controller->handleWindowMessage(
            hwnd,
            message,
            wParam,
            lParam
        );
    }

    return DefWindowProcW(
        hwnd,
        message,
        wParam,
        lParam
    );
}
