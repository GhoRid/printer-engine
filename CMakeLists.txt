cmake_minimum_required(VERSION 3.20)

project(
    printer_engine
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ninja / Makefile 계열에서 compile_commands.json 생성
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# MSVC에서 소스 파일을 UTF-8로 처리
if(MSVC)
    add_compile_options(/utf-8)
endif()

# 엔진 내부 공통 헤더 경로
set(
    PRINTER_ENGINE_INTERNAL_INCLUDE_DIRS

    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/serial
    ${CMAKE_CURRENT_SOURCE_DIR}/src/print
    ${CMAKE_CURRENT_SOURCE_DIR}/src/printer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/printer/bixolon
    ${CMAKE_CURRENT_SOURCE_DIR}/src/printer/epson
)

# printer_engine 핵심 라이브러리
add_library(
    printer_engine
    STATIC

    src/printer_engine.cpp
    src/bitmap.cpp
    src/layout_engine.cpp
    src/receipt_engine.cpp

    src/printer/printer_backend.cpp
    src/printer/printer_factory.cpp
    src/printer/printer_detect.cpp

    src/printer/bixolon/bixolon_backend.cpp
    src/printer/epson/epson_backend.cpp

    src/print/print_router.cpp
    src/print/receipt_print.cpp
    src/print/access_pass_print.cpp
)

# Node.js addon 같은 공유 라이브러리에서도 사용 가능하게 설정
set_target_properties(
    printer_engine
    PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# printer_engine 헤더 경로
target_include_directories(
    printer_engine

    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include

    PRIVATE
    ${PRINTER_ENGINE_INTERNAL_INCLUDE_DIRS}
)

# 운영체제별 시리얼 구현
if(WIN32)
    target_sources(
        printer_engine
        PRIVATE
        src/serial/serial_win.cpp
    )
elseif(APPLE)
    target_sources(
        printer_engine
        PRIVATE
        src/serial/serial_posix.cpp
    )
elseif(UNIX)
    target_sources(
        printer_engine
        PRIVATE
        src/serial/serial_posix.cpp
    )
else()
    message(FATAL_ERROR "Unsupported operating system")
endif()

# cmake-js로 실행할 때만 Node.js addon 생성
if(DEFINED CMAKE_JS_VERSION)
    add_library(
        printer_engine_node
        SHARED

        node/addon.cpp
        ${CMAKE_JS_SRC}
    )

    set_target_properties(
        printer_engine_node
        PROPERTIES
        PREFIX ""
        SUFFIX ".node"
    )

    target_include_directories(
        printer_engine_node
        PRIVATE

        ${CMAKE_JS_INC}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${PRINTER_ENGINE_INTERNAL_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/node
        ${CMAKE_CURRENT_SOURCE_DIR}/node_modules/node-addon-api
    )

    target_compile_definitions(
        printer_engine_node
        PRIVATE
        NAPI_VERSION=8
    )

    target_link_libraries(
        printer_engine_node
        PRIVATE

        printer_engine
        ${CMAKE_JS_LIB}
    )
endif()

# Windows GUI 앱
if(
    WIN32
    AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/app/main.cpp"
)
    add_executable(
        printer_engine_app
        WIN32

        app/main.cpp
        app/controller/app_controller.cpp
        app/settings/app_settings.cpp
        app/server/local_server.cpp
        app/resources/app.rc
    )

    target_include_directories(
        printer_engine_app
        PRIVATE

        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${PRINTER_ENGINE_INTERNAL_INCLUDE_DIRS}

        ${CMAKE_CURRENT_SOURCE_DIR}/app
        ${CMAKE_CURRENT_SOURCE_DIR}/app/controller
        ${CMAKE_CURRENT_SOURCE_DIR}/app/settings
        ${CMAKE_CURRENT_SOURCE_DIR}/app/server
        ${CMAKE_CURRENT_SOURCE_DIR}/app/resources
    )

    target_compile_definitions(
        printer_engine_app
        PRIVATE

        UNICODE
        _UNICODE
    )

    target_link_libraries(
        printer_engine_app
        PRIVATE

        printer_engine

        user32
        shell32
        ws2_32
    )
endif()

# 기본 테스트
add_executable(
    printer_engine_test
    tests/main.cpp
)

target_include_directories(
    printer_engine_test
    PRIVATE

    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PRINTER_ENGINE_INTERNAL_INCLUDE_DIRS}
)

target_link_libraries(
    printer_engine_test
    PRIVATE
    printer_engine
)

# print router 테스트
add_executable(
    print_router_test
    tests/print_router_test.cpp
)

target_include_directories(
    print_router_test
    PRIVATE

    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PRINTER_ENGINE_INTERNAL_INCLUDE_DIRS}
)

target_link_libraries(
    print_router_test
    PRIVATE
    printer_engine
)

# CMake 테스트 기능 활성화
enable_testing()

add_test(
    NAME printer_engine_test
    COMMAND printer_engine_test
)

add_test(
    NAME print_router_test
    COMMAND print_router_test
)