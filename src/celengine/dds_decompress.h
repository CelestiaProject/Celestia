#ifndef _DDS_DECOMPRESS_H_
#define _DDS_DECOMPRESS_H_

#ifdef __cplusplus
extern "C" {
#endif

void DecompressBlockDXT1(uint32_t x, uint32_t y, uint32_t width,
    const uint8_t* blockStorage,
    int transparent0, int* simpleAlpha, int *complexAlpha,
    uint32_t* image);

void DecompressBlockDXT3(uint32_t x, uint32_t y, uint32_t width,
    const uint8_t* blockStorage,
    int transparent0, int* simpleAlpha, int *complexAlpha,
    uint32_t* image);

void DecompressBlockDXT5(uint32_t x, uint32_t y, uint32_t width,
    const uint8_t* blockStorage,
    int transparent0, int* simpleAlpha, int *complexAlpha,
    uint32_t* image);

#ifdef __cplusplus
}
#endif

#endif // _DDS_DECOMPRESS_H_
