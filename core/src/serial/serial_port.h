#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H

#include <cstddef>  //크기나 메모리 관련 기본 타입들을 제공 (C + stddef = standard definitions)
#include <string>
#include <vector>   //여러 개의 포트 이름을 목록 형태로 저장하기 위해 사용

class SerialPort {
public:
    SerialPort();
    ~SerialPort();

    // 현재 OS에서 사용 가능한 시리얼 포트 목록을 조회한다.
    static std::vector<std::string> listPorts();

    // 시리얼 포트를 연다.
    bool open(
        const std::string& port,
        int baudRate,
        int dataBits,
        int stopBits,
        int parity
    );

    // 프린터로 데이터를 전송한다.
    bool write(const void* data, std::size_t size);

    // 프린터 응답을 timeout까지 읽는다. timeout이면 0을 반환한다.
    std::size_t read(void* data, std::size_t size);

    // 자동 감지 전에 남아 있는 수신 데이터를 비운다.
    void discardInput();

    // 시리얼 포트를 닫는다.
    void close();

    // 현재 포트가 열려 있는지 확인한다.
    bool isOpen() const;

private:

#ifdef _WIN32
    // Windows의 HANDLE을 저장한다.
    void* handle_;
#else
    // macOS / Linux의 파일 디스크립터.
    int fd_;
#endif
};

#endif
