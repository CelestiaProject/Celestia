#pragma once

#include <cstddef>
#include <cstdint>

namespace celestia::engine
{

/**
 * @brief Decompresses one block of a DXT1 texture.
 * Decompresses one block of a DXT1 texture and stores the resulting pixels at the appropriate offset in 'image'.
 *
 * @param x - x-coordinate of the first pixel in the block.
 * @param y - y-coordinate of the first pixel in the block.
 * @param width - width of the texture being decompressed.
 * @param blockStorage - pointer to the block to decompress.
 * @param image - pointer to image where the decompressed pixel data should be stored.
*/
void DecompressBlockDXT1(std::uint32_t x, std::uint32_t y, std::uint32_t width,
                         const std::uint8_t *blockStorage, bool transparent0,
                         std::uint32_t *image);

/*
 * @brief Decompresses one block of a DXT3 texture.
 * Decompresses one block of a DXT3 texture and stores the resulting pixels at the appropriate offset in 'image'.
 *
 * @param x - x-coordinate of the first pixel in the block.
 * @param y - y-coordinate of the first pixel in the block.
 * @param height - height of the texture being decompressed.
 * @param blockStorage - pointer to the block to decompress.
 * @param image - pointer to image where the decompressed pixel data should be stored.
*/
void DecompressBlockDXT3(std::uint32_t x, std::uint32_t y, std::uint32_t width,
                         const std::uint8_t *blockStorage, bool transparent0,
                         std::uint32_t *image);

/**
 * @brief Decompresses one block of a DXT5 texture.
 * Decompresses one block of a DXT5 texture and stores the resulting pixels at the appropriate offset in 'image'.
 *
 * @param x - x-coordinate of the first pixel in the block.
 * @param y - y-coordinate of the first pixel in the block.
 * @param width - width of the texture being decompressed.
 * @param blockStorage - pointer to the block to decompress.
 * @param image - pointer to image where the decompressed pixel data should be stored.
*/
void DecompressBlockDXT5(std::uint32_t x, std::uint32_t y, std::uint32_t width,
                         const std::uint8_t *blockStorage, bool transparent0,
                         std::uint32_t *image);

} // namespace celestia::engine
