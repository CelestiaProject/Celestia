#pragma once

#include <cstdint>

void DecompressBlockDXT1(std::uint32_t x, std::uint32_t y, std::uint32_t width,
    const std::uint8_t* blockStorage,
    bool transparent0,
    std::uint32_t* image);

void DecompressBlockDXT3(std::uint32_t x, std::uint32_t y, std::uint32_t width,
    const std::uint8_t* blockStorage,
    bool transparent0,
    std::uint32_t* image);

void DecompressBlockDXT5(std::uint32_t x, std::uint32_t y, std::uint32_t width,
    const std::uint8_t* blockStorage,
    bool transparent0,
    std::uint32_t* image);
