#ifndef BITMAP_H
#define BITMAP_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace pe {

class Bitmap
{
public:
    Bitmap(int width, int height);

    // 비트맵 전체 가로/세로 크기
    int width() const;
    int height() const;

    // 한 줄이 차지하는 byte 수
    int bytesPerRow() const;

    // 전체 비트맵 초기화
    // true = 검정, false = 흰색
    void clear(bool black = false);

    // 특정 픽셀 설정
    void setPixel(int x, int y, bool black = true);

    // 특정 픽셀 값 확인
    bool getPixel(int x, int y) const;

    // 프린터에 전달할 실제 byte 데이터
    const std::vector<std::uint8_t>& data() const;

private:
    int bitmapWidth;
    int bitmapHeight;
    int rowBytes;

    std::vector<std::uint8_t> bitmapData;

    // 좌표가 비트맵 내부인지 확인
    bool isInside(int x, int y) const;
};

} // namespace pe

#endif