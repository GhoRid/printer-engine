cmake_minimum_required(VERSION 3.20)

project(
    printer_engine
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20) # C++20 표준을 사용하도록 설정
set(CMAKE_CXX_STANDARD_REQUIRED ON) # 반드시 C++20을 지원하는 컴파일러 사용
set(CMAKE_CXX_EXTENSIONS OFF) # 컴파일러 고유 확장 기능을 끄고 표준 C++20을 사용하겠다

add_library(
    printer_engine # printer_engine이라는 라이브러리를 만들어라
    STATIC # 정적으로
    src/printer_engine.cpp # 이 소스들을 가지고
    src/bitmap.cpp
    src/bixolon.cpp
    src/layout_engine.cpp
    src/receipt_engine.cpp
)

# 운영체제에 따라서 사용할 시리얼 구현 파일을 선택
if(WIN32)
    # Windows에서는 Windows용 시리얼 구현 사용
    target_sources(
        printer_engine
        PRIVATE
        src/serial/serial_win.cpp
    )
elseif(UNIX)
    # macOS / Linux에서는 POSIX용 시리얼 구현 사용
    target_sources(
        printer_engine
        PRIVATE
        src/serial/serial_posix.cpp
    )
else()
    message(FATAL_ERROR "Unsupported operating system")
endif()

target_include_directories( # 헤더 파일 경로 찾기
    printer_engine
    PUBLIC # 자기 자신도, 다른 타겟도 이 경로를 공유
    ${CMAKE_CURRENT_SOURCE_DIR}/include # CMAKE_CURRENT_SOURCE_DIR : 루트 경로
    PRIVATE # printer_engine 내부에서만 사용하는 헤더 경로
    ${CMAKE_CURRENT_SOURCE_DIR}/src/serial # serial_port.h가 들어있는 경로
)

# 실제 실행파일을 만드는 부분
add_executable(
    printer_engine_test # 실행파일 이름 printer_engine_test
    tests/main.cpp # 이 소스를 통해서
)

# 테스트 코드에서 엔진 내부의 serial_port.h를 찾을 수 있게 경로 추가
target_include_directories(
    printer_engine_test
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/serial # serial_port.h가 private에 있으므로 테스트 코드에서 찾을 수 있도록 include 경로 추가
)

target_link_libraries(
    printer_engine_test # printer_engine_test 실행파일이
    PRIVATE
    printer_engine # printer_engine 라이브러리를 사용하도록 해라
)

# CMake의 테스트 기능 활성화
enable_testing()

# ctest 명령으로 printer_engine_test를 실행할 수 있게 등록
add_test(
    NAME printer_engine_test
    COMMAND printer_engine_test
)