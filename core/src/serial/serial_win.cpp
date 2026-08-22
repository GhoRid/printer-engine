#include "serial_port.h"

#ifdef _WIN32

#include <windows.h> // Windows API 사용 (COM 포트, HANDLE, WriteFile 등)
#include <algorithm> // 정렬 기능 제공 (std::sort)

SerialPort::SerialPort()
    // Windows에서는 file descriptor 대신 HANDLE 사용
    // nullptr = "아직 아무 포트도 안 열려 있음"
    : handle_(nullptr)
{
}

SerialPort::~SerialPort()
{
    close();
}


// Windows COM 포트 조회 로직
std::vector<std::string> SerialPort::listPorts()
{
    std::vector<std::string> ports;

    /*
     * Windows에서는 등록된 시리얼 포트 정보가
     * Registry의 아래 위치에 저장된다.
     *
     * HKEY_LOCAL_MACHINE
     * \HARDWARE\DEVICEMAP\SERIALCOMM
     *
     * 예:
     * COM3
     * COM8
     * COM10
     */

    HKEY key;

    // 시리얼 포트 정보가 저장된 Registry 열기
    if (
        RegOpenKeyExA(
            HKEY_LOCAL_MACHINE,
            "HARDWARE\\DEVICEMAP\\SERIALCOMM",
            0,
            KEY_READ,
            &key
        ) != ERROR_SUCCESS
    ) {
        return ports;
    }

    DWORD index = 0;

    // Registry에 등록된 COM 포트를 하나씩 조회
    while (true) {
        char valueName[256];
        BYTE data[256];

        DWORD valueNameSize = sizeof(valueName);
        DWORD dataSize = sizeof(data);
        DWORD type = 0;

        LONG result = RegEnumValueA(
            key,
            index,
            valueName,
            &valueNameSize,
            nullptr,
            &type,
            data,
            &dataSize
        );

        // 더 이상 조회할 포트가 없으면 종료
        if (result == ERROR_NO_MORE_ITEMS) {
            break;
        }

        // 정상적으로 문자열 형태의 포트 정보를 가져왔다면 목록에 추가
        if (
            result == ERROR_SUCCESS &&
            type == REG_SZ &&
            dataSize > 0
        ) {
            data[sizeof(data) - 1] = '\0';

            const std::string portName =
                reinterpret_cast<const char*>(data);

            ports.push_back(portName);
        }

        index++;
    }

    // 열었던 Registry 닫기
    RegCloseKey(key);

    // COM 포트 이름 순서대로 정렬
    std::sort(
        ports.begin(),
        ports.end()
    );

    // 혹시 같은 포트가 중복되어 있다면 제거
    ports.erase(
        std::unique(
            ports.begin(),
            ports.end()
        ),
        ports.end()
    );

    return ports;
}


bool SerialPort::open(
    const std::string& port,
    int baudRate,
    int dataBits,
    int stopBits,
    int parity
)
{
    // 기존에 열려 있는 포트가 있다면 먼저 닫기
    close();

    /*
     * Windows COM 포트 경로 변환
     *
     * COM3
     * →
     * \\.\COM3
     *
     * COM10 이상에서도 정상적으로 포트를 열기 위해
     * Windows 장치 경로 형식으로 변환
     */
    std::string devicePath = "\\\\.\\" + port;

    /*
     * COM 포트 열기
     */
    HANDLE handle = CreateFileA(
        devicePath.c_str(),
        GENERIC_READ | GENERIC_WRITE, // 읽기 + 쓰기 가능
        0,                            // 다른 프로그램과 포트 공유하지 않음
        nullptr,                      // 기본 보안 설정 사용
        OPEN_EXISTING,                // 이미 존재하는 COM 장치를 열기
        0,                            // 일반 동기 방식 사용
        nullptr                       // 템플릿 핸들 사용 안 함
    );

    // 포트를 열지 못했으면 false 반환
    if (handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    /*
     * DCB = Device Control Block
     *
     * Windows에서 시리얼 포트의
     * baud rate, data bits, stop bits, parity 등을 설정하는 구조체
     */
    DCB dcb{};

    dcb.DCBlength = sizeof(DCB);

    // 현재 COM 포트 설정값 가져오기
    if (!GetCommState(handle, &dcb)) {
        CloseHandle(handle);
        return false;
    }

    /*
     * Baud Rate
     */
    dcb.BaudRate =
        static_cast<DWORD>(baudRate);

    /*
     * Data Bits
     */
    dcb.ByteSize =
        static_cast<BYTE>(dataBits);

    /*
     * Stop Bits
     *
     * 1 = 1 stop bit
     * 2 = 2 stop bits
     */
    if (stopBits == 2) {
        dcb.StopBits = TWOSTOPBITS;
    } else {
        dcb.StopBits = ONESTOPBIT;
    }

    /*
     * Parity
     *
     * 0 = None
     * 1 = Odd
     * 2 = Even
     */
    switch (parity) {
        case 1:
            dcb.Parity = ODDPARITY;
            dcb.fParity = TRUE;
            break;

        case 2:
            dcb.Parity = EVENPARITY;
            dcb.fParity = TRUE;
            break;

        case 0:
        default:
            dcb.Parity = NOPARITY;
            dcb.fParity = FALSE;
            break;
    }

    // 위에서 설정한 시리얼 통신 설정값을 COM 포트에 적용
    if (!SetCommState(handle, &dcb)) {
        CloseHandle(handle);
        return false;
    }

    /*
     * Timeout 설정
     *
     * read / write 작업이 너무 오래 멈춰 있지 않도록
     * 제한 시간을 설정
     */
    COMMTIMEOUTS timeouts{};

    // 읽기 간격 timeout
    timeouts.ReadIntervalTimeout = 50;

    // 전체 읽기 timeout 계산에 사용
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;

    // 전체 쓰기 timeout 계산에 사용
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    // timeout 설정 적용
    if (!SetCommTimeouts(handle, &timeouts)) {
        CloseHandle(handle);
        return false;
    }

    // 정상적으로 열린 COM 포트 HANDLE 저장
    handle_ = handle;

    return true;
}

bool SerialPort::write(
    const void* data,
    std::size_t size
)
{
    // 포트가 열려 있지 않으면 전송 불가
    if (!isOpen()) {
        return false;
    }

    // void* 형태로 저장된 HANDLE을
    // Windows HANDLE 타입으로 변환
    HANDLE handle =
        static_cast<HANDLE>(handle_);

    // 전달받은 데이터를 byte 단위로 접근하기 위해 변환
    const unsigned char* buffer =
        static_cast<const unsigned char*>(data);

    // 지금까지 실제로 전송한 byte 수
    std::size_t totalWritten = 0;

    /*
     * WriteFile이 요청한 데이터를
     * 한 번에 전부 보내지 못할 수도 있기 때문에
     * 전체 데이터가 전송될 때까지 반복
     */
    while (totalWritten < size) {
        DWORD written = 0;

        /*
         * COM 포트로 데이터 전송
         */
        if (!WriteFile(
                handle,
                buffer + totalWritten,
                static_cast<DWORD>(
                    size - totalWritten
                ),
                &written,
                nullptr
            )) {
            return false;
        }

        // 성공했는데 전송된 데이터가 0 byte라면
        // 더 이상 진행할 수 없으므로 실패 처리
        if (written == 0) {
            return false;
        }

        // 실제로 전송된 byte 수 누적
        totalWritten += written;
    }

    /*
     * Windows 내부 버퍼에 남아 있는 데이터를
     * 실제 장치로 보내도록 요청
     */
    FlushFileBuffers(handle);

    return true;
}

void SerialPort::close()
{
    // 포트가 열려 있을 때만 닫기
    if (handle_ != nullptr) {
        HANDLE handle =
            static_cast<HANDLE>(handle_);

        // Windows COM 포트 HANDLE 닫기
        CloseHandle(handle);

        // 다시 "포트가 닫힌 상태"로 초기화
        handle_ = nullptr;
    }
}

bool SerialPort::isOpen() const
{
    // nullptr이 아니면 현재 포트가 열려 있음
    return handle_ != nullptr;
}

#endif