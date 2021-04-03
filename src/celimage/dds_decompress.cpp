#include <stddef.h>
#include <stdint.h>

/*
DXT1/DXT3/DXT5 texture decompression

The original code is from Benjamin Dobell, see below for details. Compared to
the original this one adds DXT3 decompression, is valid C89, and is x64
compatible as it uses fixed size integers everywhere. It also uses a different
PackRGBA order.

---

Copyright (c) 2012, Matth�us G. "Anteru" Chajdas (http://anteru.net)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

---

Copyright (C) 2009 Benjamin Dobell, Glass Echidna

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

---
*/
constexpr uint32_t PackRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return r | (g << 8) | (b << 16) | (a << 24);
}

static void DecompressBlockDXT1Internal(const uint8_t* block,
                                        uint32_t* output,
                                        uint32_t outputStride,
                                        bool transparent0,
                                        const uint8_t* alphaValues)
{
    uint32_t temp, code;

    uint16_t color0, color1;
    uint8_t r0, g0, b0, r1, g1, b1;

    int i, j;

    color0 = *(const uint16_t*)(block);
    color1 = *(const uint16_t*)(block + 2);

    temp = (color0 >> 11) * 255 + 16;
    r0 = (uint8_t)((temp / 32 + temp) / 32);
    temp = ((color0 & 0x07E0) >> 5) * 255 + 32;
    g0 = (uint8_t)((temp / 64 + temp) / 64);
    temp = (color0 & 0x001F) * 255 + 16;
    b0 = (uint8_t)((temp / 32 + temp) / 32);

    temp = (color1 >> 11) * 255 + 16;
    r1 = (uint8_t)((temp / 32 + temp) / 32);
    temp = ((color1 & 0x07E0) >> 5) * 255 + 32;
    g1 = (uint8_t)((temp / 64 + temp) / 64);
    temp = (color1 & 0x001F) * 255 + 16;
    b1 = (uint8_t)((temp / 32 + temp) / 32);

    code = *(const uint32_t*)(block + 4);

    if (color0 > color1)
    {
        for (j = 0; j < 4; ++j)
        {
            for (i = 0; i < 4; ++i)
            {
                uint32_t finalColor, positionCode;
                uint8_t alpha;

                alpha = alphaValues[j * 4 + i];

                finalColor = 0;
                positionCode = (code >> 2 * (4 * j + i)) & 0x03;

                switch (positionCode)
                {
                case 0:
                    finalColor = PackRGBA(r0, g0, b0, alpha);
                    break;
                case 1:
                    finalColor = PackRGBA(r1, g1, b1, alpha);
                    break;
                case 2:
                    finalColor = PackRGBA((2 * r0 + r1) / 3, (2 * g0 + g1) / 3,
                                          (2 * b0 + b1) / 3, alpha);
                    break;
                case 3:
                    finalColor = PackRGBA((r0 + 2 * r1) / 3, (g0 + 2 * g1) / 3,
                                          (b0 + 2 * b1) / 3, alpha);
                    break;
                }
                if (transparent0 && (finalColor == PackRGBA(0, 0, 0, 0xff)))
                {
                    alpha = 0;
                    finalColor = 0;
                }
                output[j * outputStride + i] = finalColor;
            }
        }
    }
    else
    {
        for (j = 0; j < 4; ++j)
        {
            for (i = 0; i < 4; ++i)
            {
                uint32_t finalColor, positionCode;
                uint8_t alpha;

                alpha = alphaValues[j * 4 + i];

                finalColor = 0;
                positionCode = (code >> 2 * (4 * j + i)) & 0x03;

                switch (positionCode)
                {
                case 0:
                    finalColor = PackRGBA(r0, g0, b0, alpha);
                    break;
                case 1:
                    finalColor = PackRGBA(r1, g1, b1, alpha);
                    break;
                case 2:
                    finalColor = PackRGBA((r0 + r1) / 2, (g0 + g1) / 2, (b0 + b1) / 2, alpha);
                    break;
                case 3:
                    finalColor = PackRGBA(0, 0, 0, alpha);
                    break;
                }

                if (transparent0 && (finalColor == PackRGBA(0, 0, 0, 0xff)))
                {
                    alpha = 0;
                    finalColor = 0;
                }

                output[j * outputStride + i] = finalColor;
            }
        }
    }
}

/*
void DecompressBlockDXT1(): Decompresses one block of a DXT1 texture and stores the resulting pixels at the appropriate offset in 'image'.

uint32_t x:                     x-coordinate of the first pixel in the block.
uint32_t y:                     y-coordinate of the first pixel in the block.
uint32_t width:                 width of the texture being decompressed.
const uint8_t *blockStorage:    pointer to the block to decompress.
uint32_t *image:                pointer to image where the decompressed pixel data should be stored.
*/
void DecompressBlockDXT1(uint32_t x,
                         uint32_t y,
                         uint32_t width,
                         const uint8_t* blockStorage,
                         bool transparent0,
                         uint32_t* image)
{
    static const uint8_t const_alpha[] = { 255, 255, 255, 255, 255, 255,
                                           255, 255, 255, 255, 255, 255,
                                           255, 255, 255, 255 };

    DecompressBlockDXT1Internal(blockStorage, image + x + (y * width), width,
                                transparent0, const_alpha);
}

/*
void DecompressBlockDXT5(): Decompresses one block of a DXT5 texture and stores the resulting pixels at the appropriate offset in 'image'.

uint32_t x:                     x-coordinate of the first pixel in the block.
uint32_t y:                     y-coordinate of the first pixel in the block.
uint32_t width:                 width of the texture being decompressed.
const uint8_t *blockStorage:    pointer to the block to decompress.
uint32_t *image:                pointer to image where the decompressed pixel data should be stored.
*/
void DecompressBlockDXT5(uint32_t x,
                         uint32_t y,
                         uint32_t width,
                         const uint8_t* blockStorage,
                         bool transparent0,
                         uint32_t* image)
{
    uint8_t alpha0, alpha1;
    const uint8_t* bits;
    uint32_t alphaCode1;
    uint16_t alphaCode2;

    uint16_t color0, color1;
    uint8_t r0, g0, b0, r1, g1, b1;

    int i, j;

    uint32_t temp, code;

    alpha0 = *(blockStorage);
    alpha1 = *(blockStorage + 1);

    bits = blockStorage + 2;
    alphaCode1 = bits[2] | (bits[3] << 8) | (bits[4] << 16) | (bits[5] << 24);
    alphaCode2 = bits[0] | (bits[1] << 8);

    color0 = *(const uint16_t*)(blockStorage + 8);
    color1 = *(const uint16_t*)(blockStorage + 10);

    temp = (color0 >> 11) * 255 + 16;
    r0 = (uint8_t)((temp / 32 + temp) / 32);
    temp = ((color0 & 0x07E0) >> 5) * 255 + 32;
    g0 = (uint8_t)((temp / 64 + temp) / 64);
    temp = (color0 & 0x001F) * 255 + 16;
    b0 = (uint8_t)((temp / 32 + temp) / 32);

    temp = (color1 >> 11) * 255 + 16;
    r1 = (uint8_t)((temp / 32 + temp) / 32);
    temp = ((color1 & 0x07E0) >> 5) * 255 + 32;
    g1 = (uint8_t)((temp / 64 + temp) / 64);
    temp = (color1 & 0x001F) * 255 + 16;
    b1 = (uint8_t)((temp / 32 + temp) / 32);

    code = *(const uint32_t*)(blockStorage + 12);

    for (j = 0; j < 4; j++)
    {
        for (i = 0; i < 4; i++)
        {
            uint8_t finalAlpha;
            int alphaCode, alphaCodeIndex;
            uint8_t colorCode;
            uint32_t finalColor;

            alphaCodeIndex = 3 * (4 * j + i);
            if (alphaCodeIndex <= 12)
            {
                alphaCode = (alphaCode2 >> alphaCodeIndex) & 0x07;
            }
            else if (alphaCodeIndex == 15)
            {
                alphaCode = (alphaCode2 >> 15) | ((alphaCode1 << 1) & 0x06);
            }
            else /* alphaCodeIndex >= 18 && alphaCodeIndex <= 45 */
            {
                alphaCode = (alphaCode1 >> (alphaCodeIndex - 16)) & 0x07;
            }

            if (alphaCode == 0)
            {
                finalAlpha = alpha0;
            }
            else if (alphaCode == 1)
            {
                finalAlpha = alpha1;
            }
            else
            {
                if (alpha0 > alpha1)
                {
                    finalAlpha = (uint8_t)(((8 - alphaCode) * alpha0 + (alphaCode - 1) * alpha1) / 7);
                }
                else
                {
                    if (alphaCode == 6)
                    {
                        finalAlpha = 0;
                    }
                    else if (alphaCode == 7)
                    {
                        finalAlpha = 255;
                    }
                    else
                    {
                        finalAlpha = (uint8_t)(((6 - alphaCode) * alpha0 + (alphaCode - 1) * alpha1) / 5);
                    }
                }
            }

            colorCode = (code >> 2 * (4 * j + i)) & 0x03;
            finalColor = 0;

            switch (colorCode)
            {
            case 0:
                finalColor = PackRGBA(r0, g0, b0, finalAlpha);
                break;
            case 1:
                finalColor = PackRGBA(r1, g1, b1, finalAlpha);
                break;
            case 2:
                finalColor = PackRGBA((2 * r0 + r1) / 3, (2 * g0 + g1) / 3,
                                      (2 * b0 + b1) / 3, finalAlpha);
                break;
            case 3:
                finalColor = PackRGBA((r0 + 2 * r1) / 3, (g0 + 2 * g1) / 3,
                                      (b0 + 2 * b1) / 3, finalAlpha);
                break;
            }

            image[i + x + (width * (y + j))] = finalColor;
        }
    }
}

/*
void DecompressBlockDXT3(): Decompresses one block of a DXT3 texture and stores the resulting pixels at the appropriate offset in 'image'.

uint32_t x:                     x-coordinate of the first pixel in the block.
uint32_t y:                     y-coordinate of the first pixel in the block.
uint32_t height:                height of the texture being decompressed.
const uint8_t *blockStorage:    pointer to the block to decompress.
uint32_t *image:                pointer to image where the decompressed pixel data should be stored.
*/
void DecompressBlockDXT3(uint32_t x,
                         uint32_t y,
                         uint32_t width,
                         const uint8_t* blockStorage,
                         bool transparent0,
                         uint32_t* image)
{
    int i;

    uint8_t alphaValues[16] = { 0 };

    for (i = 0; i < 4; ++i)
    {
        const uint16_t* alphaData = (const uint16_t*)(blockStorage);

        alphaValues[i * 4 + 0] = (((*alphaData) >> 0) & 0xF) * 17;
        alphaValues[i * 4 + 1] = (((*alphaData) >> 4) & 0xF) * 17;
        alphaValues[i * 4 + 2] = (((*alphaData) >> 8) & 0xF) * 17;
        alphaValues[i * 4 + 3] = (((*alphaData) >> 12) & 0xF) * 17;

        blockStorage += 2;
    }

    DecompressBlockDXT1Internal(blockStorage, image + x + (y * width), width,
                                transparent0, alphaValues);
}
