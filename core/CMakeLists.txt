cmake_minimum_required(VERSION 3.20)

project(
    printer_engine
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(
    printer_engine
    STATIC
    src/printer_engine.cpp
)

target_include_directories(
    printer_engine
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(
    printer_engine_test
    tests/main.cpp
)

target_link_libraries(
    printer_engine_test
    PRIVATE
    printer_engine
)