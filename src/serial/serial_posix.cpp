#include "serial_port.h"

#ifndef _WIN32

#include <cerrno>    //에러 번호 관련 기능 제공 (C + errno = error number)
#include <fcntl.h>   //파일/장치 제어 관련 기능 제공 (fcntl = file control)
#include <termios.h> //터미널/시리얼 통신 설정 기능 제공 (terminal I/O)
#include <unistd.h>  //Unix 기본 시스템 함수 제공 (read, write, close 등)

#include <algorithm>  //sort() 같은 정렬 기능 제공
#include <dirent.h>    //디렉토리 내부 파일/장치 목록 조회

SerialPort::SerialPort()
    //file descriptor = 열린 파일이나 장치를 가리키는 번호. 초기값 -1 = "아직 아무 포트도 안 열려 있음"
    //포트를 열면 0, 1, 2, 3, 4 ... 같은 값들
    : fd_(-1) 
{
}

SerialPort::~SerialPort()
{
    close();
}

static speed_t getBaudRate(int baudRate)
{
    switch (baudRate) {
        case 9600:
            return B9600;
        case 19200:
            return B19200;
        case 38400:
            return B38400;
        case 57600:
            return B57600;
        case 115200:
            return B115200;
        default:
            return B9600;
    }
}

// macOS / Linux용 포트 조회 로직
std::vector<std::string> SerialPort::listPorts()
{
    std::vector<std::string> ports;

    // macOS / Linux에서는 장치들이 /dev 폴더 아래에 파일처럼 등록된다.
    DIR* directory = opendir("/dev");

    // /dev 폴더를 열지 못했다면 빈 목록 반환
    if (directory == nullptr) {
        return ports;
    }

    dirent* entry = nullptr;

    // /dev 폴더 내부의 파일/장치 목록을 하나씩 조회
    while ((entry = readdir(directory)) != nullptr) {
        // 예:
        // /dev/cu.usbserial-110
        // →
        // cu.usbserial-110
        const std::string name =
            entry->d_name;

#ifdef __APPLE__

        /*
         * macOS
         *
         * 일반적으로 시리얼 통신에는
         * /dev/tty.*보다 /dev/cu.*를 사용한다.
         *
         * 예:
         * /dev/cu.usbserial-110
         * /dev/cu.usbmodem101
         * /dev/cu.wchusbserial1234
         * /dev/cu.SLAB_USBtoUART
         */

        // 이름이 "cu."로 시작하지 않으면 시리얼 후보에서 제외
        if (name.rfind("cu.", 0) != 0) {
            continue;
        }

        // macOS 기본 Bluetooth / Debug 가상 포트는 제외
        if (
            name == "cu.Bluetooth-Incoming-Port" ||
            name == "cu.debug-console"
        ) {
            continue;
        }

        // 실제 전체 경로를 목록에 추가
        ports.push_back(
            "/dev/" + name
        );

#else
        /*
         * Linux
         *
         * USB Serial:
         * /dev/ttyUSB0
         *
         * USB CDC:
         * /dev/ttyACM0
         *
         * 일반 Serial:
         * /dev/ttyS0
         */

        if (
            name.rfind("ttyUSB", 0) == 0 ||
            name.rfind("ttyACM", 0) == 0 ||
            name.rfind("ttyS", 0) == 0
        ) {
            ports.push_back(
                "/dev/" + name
            );
        }

#endif
    }

    // 열었던 /dev 폴더 닫기
    closedir(directory);

    // 포트 목록을 이름 순서대로 정렬
    std::sort(
        ports.begin(),
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
    close();

    // 예:
    // /dev/tty.usbserial
    // /dev/ttyUSB0
    fd_ = ::open(
        port.c_str(),
        O_RDWR | O_NOCTTY | O_SYNC  //읽기/쓰기, 터미널 제어 없음, 동기식 I/O
    );

    // 열린 포트가 없으면 false 반환
    if (fd_ < 0) {
        return false;
    }

    termios tty{};

    if (tcgetattr(fd_, &tty) != 0) {
        close();
        return false;
    }

    /*
     * Baud Rate
     */
    speed_t speed = getBaudRate(baudRate);

    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    /*
     * Data Bits
     */
    tty.c_cflag &= ~CSIZE;

    switch (dataBits) {
        case 5:
            tty.c_cflag |= CS5;
            break;
        case 6:
            tty.c_cflag |= CS6;
            break;
        case 7:
            tty.c_cflag |= CS7;
            break;
        case 8:
        default:
            tty.c_cflag |= CS8;
            break;
    }

    /*
     * Stop Bits
     */
    if (stopBits == 2) {
        tty.c_cflag |= CSTOPB;
    } else {
        tty.c_cflag &= ~CSTOPB;
    }

    /*
     * Parity
     *
     * 0 = None
     * 1 = Odd
     * 2 = Even
     */
    if (parity == 0) {
        tty.c_cflag &= ~PARENB;
    } else {
        tty.c_cflag |= PARENB;
        if (parity == 1) {
            // Odd
            tty.c_cflag |= PARODD;
        } else {
            // Even
            tty.c_cflag &= ~PARODD;
        }
    }


    /*
     * 일반적인 Raw Serial 설정
     */
    tty.c_cflag |= CLOCAL;
    tty.c_cflag |= CREAD;

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    tty.c_oflag &= ~OPOST;


    /*
     * read timeout 설정
     */

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 2;

    if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
        close();
        return false;
    }

    return true;
}


bool SerialPort::write(
    const void* data,
    std::size_t size
)
{
    if (!isOpen()) {
        return false;
    }

    const unsigned char* buffer =
        static_cast<const unsigned char*>(data);

    std::size_t totalWritten = 0;

    while (totalWritten < size) {
        ssize_t written = ::write(
            fd_,
            buffer + totalWritten,
            size - totalWritten
        );

        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }

        totalWritten +=
            static_cast<std::size_t>(written);
    }

    // OS 버퍼에 남아 있는 데이터를 실제 장치로 전송할 때까지 기다림.
    tcdrain(fd_);

    return true;
}

std::size_t SerialPort::read(void* data, std::size_t size)
{
    if (!isOpen() || !data || size == 0) {
        return 0;
    }

    ssize_t bytesRead;

    do {
        bytesRead = ::read(fd_, data, size);
    } while (bytesRead < 0 && errno == EINTR);

    return bytesRead > 0 ? static_cast<std::size_t>(bytesRead) : 0;
}

void SerialPort::discardInput()
{
    if (isOpen()) {
        tcflush(fd_, TCIFLUSH);
    }
}


void SerialPort::close()
{
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}


bool SerialPort::isOpen() const
{
    return fd_ >= 0;
}


#endif
