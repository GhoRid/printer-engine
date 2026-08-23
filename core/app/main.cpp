#include <QApplication>

#include "app_controller.h"

int main(int argc, char* argv[])
{
    // Qt GUI 애플리케이션 생성
    QApplication app(argc, argv);

    // 애플리케이션 기본 정보 설정
    QApplication::setApplicationName("Printer Engine");
    QApplication::setApplicationVersion("0.1.0");
    QApplication::setOrganizationName("PrinterEngine");

    // GUI 창을 닫아도 프로그램 자체는 종료하지 않음
    // 시스템 트레이에서 백그라운드로 계속 실행하기 위해 필요
    QApplication::setQuitOnLastWindowClosed(false);

    // 프린터, GUI, 로컬 서버, 시스템 트레이를 관리하는 컨트롤러 생성
    AppController controller;

    // 앱 초기화 실패 시 프로그램 종료
    if (!controller.initialize()) {
        return 1;
    }

    // Qt 이벤트 루프 실행
    // 프로그램은 시스템 트레이에서 계속 살아있음
    return app.exec();
}