#include "bitmap.h"

#include <algorithm>
#include <stdexcept>

namespace pe {

Bitmap::Bitmap(int width, int height)
    : bitmapWidth(width),
      bitmapHeight(height),
      rowBytes((width + 7) / 8)
{
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("Bitmap width and height must be greater than 0");
    }

    bitmapData.resize(
        static_cast<std::size_t>(rowBytes) *
        static_cast<std::size_t>(bitmapHeight),
        0
    );
}

int Bitmap::width() const
{
    return bitmapWidth;
}

int Bitmap::height() const
{
    return bitmapHeight;
}

int Bitmap::bytesPerRow() const
{
    return rowBytes;
}

void Bitmap::clear(bool black)
{
    std::fill(
        bitmapData.begin(),
        bitmapData.end(),
        black ? 0xFF : 0x00
    );
}

void Bitmap::setPixel(int x, int y, bool black)
{
    if (!isInside(x, y)) {
        return;
    }

    const std::size_t byteIndex =
        static_cast<std::size_t>(y * rowBytes + x / 8);

    const int bitIndex = 7 - (x % 8);

    const std::uint8_t mask =
        static_cast<std::uint8_t>(1u << bitIndex);

    if (black) {
        bitmapData[byteIndex] |= mask;
    } else {
        bitmapData[byteIndex] &=
            static_cast<std::uint8_t>(~mask);
    }
}

bool Bitmap::getPixel(int x, int y) const
{
    if (!isInside(x, y)) {
        return false;
    }

    const std::size_t byteIndex =
        static_cast<std::size_t>(y * rowBytes + x / 8);

    const int bitIndex = 7 - (x % 8);

    const std::uint8_t mask =
        static_cast<std::uint8_t>(1u << bitIndex);

    return (bitmapData[byteIndex] & mask) != 0;
}

const std::vector<std::uint8_t>& Bitmap::data() const
{
    return bitmapData;
}

bool Bitmap::isInside(int x, int y) const
{
    return
        x >= 0 &&
        y >= 0 &&
        x < bitmapWidth &&
        y < bitmapHeight;
}

} // namespace pe