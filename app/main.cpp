#include <windows.h>

#include "app_controller.h"

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    PWSTR,
    int
)
{
    AppController controller;

    if (!controller.initialize(hInstance)) {
        MessageBoxW(
            nullptr,
            L"프로그램 초기화에 실패했습니다.",
            L"Printer Engine",
            MB_OK | MB_ICONERROR
        );

        return 1;
    }

    MSG msg{};

    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    controller.shutdown();

    return static_cast<int>(msg.wParam);
}