#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct LoadedImage {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> data;
};

bool loadImageAsMonochrome(
    const std::string& path,
    int requestedWidth,
    int threshold,
    LoadedImage& output,
    std::string& error
);
