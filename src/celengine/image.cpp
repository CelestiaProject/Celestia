// image.cpp
//
// Copyright (C) 2001, Chris Laurel
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "image.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <csetjmp>
#include <cstdio>
#include <cstring> /* for memcpy */
#include <fstream>
#include <iostream>

#ifdef TARGET_OS_MAC
#include <unistd.h>
#endif

#ifndef _WIN32
#ifndef TARGET_OS_MAC
#include <config.h>
#endif /* ! TARGET_OS_MAC */
#endif /* ! _WIN32 */

extern "C" {
#ifdef _WIN32
#include "jpeglib.h"
#else
#include <cstdio>
#include <jpeglib.h>
#endif
}

#include "png.h"

#include <celutil/basictypes.h>
#include <celutil/debug.h>
#include <celutil/filetype.h>
#include <celutil/util.h>

#include "gl.h"
#include "glext.h"
#include "celestia.h"


using namespace std;


// All rows are padded to a size that's a multiple of 4 bytes
static int pad(int n)
{
    return (n + 3) & ~0x3;
}


static int formatComponents(int fmt)
{
    switch (fmt)
    {
    case GL_RGBA:
    case GL_BGRA_EXT:
        return 4;
    case GL_RGB:
    case GL_BGR_EXT:
        return 3;
    case GL_LUMINANCE_ALPHA:
    case GL_DSDT_NV:
        return 2;
    case GL_ALPHA:
    case GL_LUMINANCE:
        return 1;

    // Compressed formats
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        return 3;
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        return 4;

    // Unknown format
    default:
        return 0;
    }
}


static int calcMipLevelSize(int fmt, int w, int h, int mip)
{
    w = max(w >> mip, 1);
    h = max(h >> mip, 1);

    switch (fmt)
    {
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        // 4x4 blocks, 8 bytes per block
        return ((w + 3) / 4) * ((h + 3) / 4) * 8;
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        // 4x4 blocks, 16 bytes per block
        return ((w + 3) / 4) * ((h + 3) / 4) * 16;
    default:
        return h * pad(w * formatComponents(fmt));
    }
}


Image::Image(int fmt, int w, int h, int mip) :
    width(w),
    height(h),
    mipLevels(mip),
    format(fmt),
    pixels(NULL)
{
    components = formatComponents(fmt);
    assert(components != 0);

    pitch = pad(w * components);

    size = 1;
    for (int i = 0; i < mipLevels; i++)
        size += calcMipLevelSize(fmt, w, h, i);
    pixels = new unsigned char[size];
}


Image::~Image()
{
    if (pixels != NULL)
        delete[] pixels;
}


int Image::getWidth() const
{
    return width;
}


int Image::getHeight() const
{
    return height;
}


int Image::getPitch() const
{
    return pitch;
}


int Image::getMipLevelCount() const
{
    return mipLevels;
}


int Image::getSize() const
{
    return size;
}


int Image::getFormat() const
{
    return format;
}


int Image::getComponents() const
{
    return components;
}


unsigned char* Image::getPixels()
{
    return pixels;
}


unsigned char* Image::getPixelRow(int mip, int row)
{
    /*int w = max(width >> mip, 1); Unused*/
    int h = max(height >> mip, 1);
    if (mip >= mipLevels || row >= h)
        return NULL;

    // Row addressing of compressed textures is not allowed
    if (isCompressed())
        return NULL;

    return getMipLevel(mip) + row * pitch;
}


unsigned char* Image::getPixelRow(int row)
{
    return getPixelRow(0, row);
}


unsigned char* Image::getMipLevel(int mip)
{
    if (mip >= mipLevels)
        return NULL;

    int offset = 0;
    for (int i = 0; i < mip; i++)
        offset += calcMipLevelSize(format, width, height, i);

    return pixels + offset;
}


int Image::getMipLevelSize(int mip) const
{
    if (mip >= mipLevels)
        return 0;
    else
        return calcMipLevelSize(format, width, height, mip);
}


bool Image::isCompressed() const
{
    switch (format)
    {
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        return true;
    default:
        return false;
    }
}


bool Image::hasAlpha() const
{
    switch (format)
    {
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
    case GL_RGBA:
    case GL_BGRA_EXT:
    case GL_LUMINANCE_ALPHA:
    case GL_ALPHA:
        return true;
    default:
        return false;
    }
}


// Convert an input height map to a normal map.  Ideally, a single channel
// input should be used.  If not, the first color channel of the input image
// is the one only one used when generating normals.  This produces the
// expected results for grayscale values in RGB images.
Image* Image::computeNormalMap(float scale, bool wrap) const
{
    // Can't do anything with compressed input; there are probably some other
    // formats that should be rejected as well . . .
    if (isCompressed())
        return NULL;

    Image* normalMap = new Image(GL_RGBA, width, height);
    if (normalMap == NULL)
        return NULL;

    unsigned char* nmPixels = normalMap->getPixels();
    int nmPitch = normalMap->getPitch();

    // Compute normals using differences between adjacent texels.
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            int i0 = i;
            int j0 = j;
            int i1 = i - 1;
            int j1 = j - 1;
            if (i1 < 0)
            {
                if (wrap)
                {
                    i1 = height - 1;
                }
                else
                {
                    i0++;
                    i1++;
                }
            }
            if (j1 < 0)
            {
                if (wrap)
                {
                    j1 = width - 1;
                }
                else
                {
                    j0++;
                    j1++;
                }
            }

            int h00 = (int) pixels[i0 * pitch + j0 * components];
            int h10 = (int) pixels[i0 * pitch + j1 * components];
            int h01 = (int) pixels[i1 * pitch + j0 * components];

            float dx = (float) (h10 - h00) * (1.0f / 255.0f) * scale;
            float dy = (float) (h01 - h00) * (1.0f / 255.0f) * scale;

            float mag = (float) sqrt(dx * dx + dy * dy + 1.0f);
            float rmag = 1.0f / mag;

            int n = i * nmPitch + j * 4;
            nmPixels[n]     = (unsigned char) (128 + 127 * dx * rmag);
            nmPixels[n + 1] = (unsigned char) (128 + 127 * dy * rmag);
            nmPixels[n + 2] = (unsigned char) (128 + 127 * rmag);
            nmPixels[n + 3] = 255;
        }
    }

    return normalMap;
}


Image* LoadImageFromFile(const string& filename)
{
    ContentType type = DetermineFileType(filename);
    Image* img = NULL;

    clog << _("Loading image from file ") << filename << '\n';

    switch (type)
    {
    case Content_JPEG:
        img = LoadJPEGImage(filename);
        break;
    case Content_BMP:
        img = LoadBMPImage(filename);
        break;
    case Content_PNG:
        img = LoadPNGImage(filename);
        break;
    case Content_DDS:
    case Content_DXT5NormalMap:
        img = LoadDDSImage(filename);
        break;
    default:
        clog << filename << _(": unrecognized or unsupported image file type.\n");
        break;
    }

    return img;
}


struct my_error_mgr
{
    struct jpeg_error_mgr pub;	// "public" fields
    jmp_buf setjmp_buffer;      // for return to caller
};

typedef struct my_error_mgr *my_error_ptr;

METHODDEF(void) my_error_exit(j_common_ptr cinfo)
{
    // cinfo->err really points to a my_error_mgr struct, so coerce pointer
    my_error_ptr myerr = (my_error_ptr) cinfo->err;

    // Always display the message.
    // We could postpone this until after returning, if we chose.
    (*cinfo->err->output_message) (cinfo);

    // Return control to the setjmp point
    longjmp(myerr->setjmp_buffer, 1);
}


Image* LoadJPEGImage(const string& filename, int)
{
    Image* img = NULL;

    // This struct contains the JPEG decompression parameters and pointers to
    // working space (which is allocated as needed by the JPEG library).
    struct jpeg_decompress_struct cinfo;

    // We use our private extension JPEG error handler.
    // Note that this struct must live as long as the main JPEG parameter
    // struct, to avoid dangling-pointer problems.
    struct my_error_mgr jerr;
    // More stuff
    JSAMPARRAY buffer;		// Output row buffer
    int row_stride;		// physical row width in output buffer
    long cont;

    // In this example we want to open the input file before doing anything else,
    // so that the setjmp() error recovery below can assume the file is open.
    // VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
    // requires it in order to read binary files.

    FILE *in;
    in = fopen(filename.c_str(), "rb");
    if (in == NULL)
        return NULL;

    // Step 1: allocate and initialize JPEG decompression object
    // We set up the normal JPEG error routines, then override error_exit.
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    // Establish the setjmp return context for my_error_exit to use.
    if (setjmp(jerr.setjmp_buffer))
    {
        // If we get here, the JPEG code has signaled an error.
        // We need to clean up the JPEG object, close the input file, and return.
        jpeg_destroy_decompress(&cinfo);
        fclose(in);
        if (img != NULL)
            delete img;

        return NULL;
    }

    // Now we can initialize the JPEG decompression object.
    jpeg_create_decompress(&cinfo);

    // Step 2: specify data source (eg, a file)
    jpeg_stdio_src(&cinfo, in);

    // Step 3: read file parameters with jpeg_read_header()
    (void) jpeg_read_header(&cinfo, TRUE);
    if (cinfo.image_width == 0 || cinfo.image_width > Image::MAX_IMAGE_DIMENSION ||
        cinfo.image_height == 0 || cinfo.image_height > Image::MAX_IMAGE_DIMENSION)
    {
        jpeg_destroy_decompress(&cinfo);
        fclose(in);
        return NULL;
    }

    // We can ignore the return value from jpeg_read_header since
    //  (a) suspension is not possible with the stdio data source, and
    //  (b) we passed TRUE to reject a tables-only JPEG file as an error.

    // Step 4: set parameters for decompression

    // In this example, we don't need to change any of the defaults set by
    // jpeg_read_header(), so we do nothing here.

    // Step 5: Start decompressor

    (void) jpeg_start_decompress(&cinfo);
    // We can ignore the return value since suspension is not possible
    // with the stdio data source.

    // We may need to do some setup of our own at this point before reading
    // the data.  After jpeg_start_decompress() we have the correct scaled
    // output image dimensions available, as well as the output colormap
    // if we asked for color quantization.
    // In this example, we need to make an output work buffer of the right size.
    // JSAMPLEs per row in output buffer
    row_stride = cinfo.output_width * cinfo.output_components;
    // Make a one-row-high sample array that will go away when done with image
    buffer = (*cinfo.mem->alloc_sarray)
        ((j_common_ptr) & cinfo, JPOOL_IMAGE, row_stride, 1);

    // Step 6: while (scan lines remain to be read)
    //             jpeg_read_scanlines(...);

    // Here we use the library's state variable cinfo.output_scanline as the
    // loop counter, so that we don't have to keep track ourselves.

    int format = GL_RGB;
    if (cinfo.output_components == 1)
        format = GL_LUMINANCE;

    img = new Image(format, cinfo.image_width, cinfo.image_height);

    // cont = cinfo.output_height - 1;
    cont = 0;
    while (cinfo.output_scanline < cinfo.output_height)
    {
        // jpeg_read_scanlines expects an array of pointers to scanlines.
        // Here the array is only one element long, but you could ask for
        // more than one scanline at a time if that's more convenient.
        (void) jpeg_read_scanlines(&cinfo, buffer, 1);

        // Assume put_scanline_someplace wants a pointer and sample count.
        // put_scanline_someplace(buffer[0], row_stride);
        memcpy(img->getPixelRow(cont), buffer[0], row_stride);
        cont++;
    }

    // Step 7: Finish decompression

    (void) jpeg_finish_decompress(&cinfo);
    // We can ignore the return value since suspension is not possible
    // with the stdio data source.

    // Step 8: Release JPEG decompression object

    // This is an important step since it will release a good deal of memory.
    jpeg_destroy_decompress(&cinfo);

    // After finish_decompress, we can close the input file.
    // Here we postpone it until after no more JPEG errors are possible,
    // so as to simplify the setjmp error logic above.  (Actually, I don't
    // think that jpeg_destroy can do an error exit, but why assume anything...

    fclose(in);

    // At this point you may want to check to see whether any corrupt-data
    // warnings occurred (test whether jerr.pub.num_warnings is nonzero).

    return img;
}


static void
PNGError(png_structp pngPtr, png_const_charp error)
{
    auto filename = static_cast<const std::string*>(png_get_error_ptr(pngPtr));
    std::clog << _("PNG error: ") << *filename << ' - ' << error << '\n';
    png_longjmp(pngPtr, static_cast<int>(true));
}

static void
PNGWarn(png_structp pngPtr, png_const_charp warning)
{
    auto filename = static_cast<const std::string*>(png_get_error_ptr(pngPtr));
    std::clog << _("PNG warning: ") << *filename << ' - ' << warning << '\n';
}

static GLenum
PNGGetGLFormat(png_structp pngPtr, png_const_infop infoPtr, int bitDepth, int colorType)
{
    if (colorType == PNG_COLOR_TYPE_PALETTE)
    {
        png_set_palette_to_rgb(pngPtr);
        if (png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS))
        {
            png_set_tRNS_to_alpha(pngPtr);
            return GL_RGBA;
        }

        return GL_RGB;
    }

    if (bitDepth < 8)
    {
        png_set_packing(pngPtr);
        if (colorType == PNG_COLOR_TYPE_GRAY)
            png_set_expand_gray_1_2_4_to_8(pngPtr);
    }
    else if (bitDepth == 16)
    {
#if PNG_LIBPNG_VER >= 10504
        png_set_scale_16(pngPtr);
#else
        png_set_strip_16(pngPtr);
#endif
    }

    if (png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS))
    {
        png_set_tRNS_to_alpha(pngPtr);
        colorType |= PNG_COLOR_MASK_ALPHA;
    }

    switch (colorType)
    {
    case PNG_COLOR_TYPE_GRAY:
        return GL_LUMINANCE;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        return GL_LUMINANCE_ALPHA;
    case PNG_COLOR_TYPE_RGB:
        return GL_RGB;
    case PNG_COLOR_TYPE_RGB_ALPHA:
        return GL_RGBA;
    default:
        png_error(pngPtr, _("Unsupported color type"));
    }
}



Image* LoadPNGImage(const std::string& filename)
{
    png_byte header[8];
    png_structp pngPtr = NULL;
    png_infop infoPtr = NULL;
    png_uint_32 width, height;
    int bitDepth, colorType;
    GLenum glFormat;
    int passes;
    int pitch;
    FILE* fp = NULL;
    Image* img = NULL;

    fp = std::fopen(filename.c_str(), "rb");
    if (!fp)
    {
        std::clog << _("Error opening image file ") << filename << '\n';
        return NULL;
    }

    std::fread(header, 1, sizeof(header), fp);
    if (png_sig_cmp(header, 0, sizeof(header)))
    {
        std::clog << _("Error: ") << filename << _(" is not a PNG file.\n");
        std::fclose(fp);
        return NULL;
    }

    pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                    const_cast<std::string*>(&filename),
                                    &PNGError,
                                    &PNGWarn);
    if (!pngPtr)
    {
        std::fclose(fp);
        return NULL;
    }

    infoPtr = png_create_info_struct(pngPtr);
    if (!infoPtr)
    {
        std::fclose(fp);
        png_destroy_read_struct(&pngPtr, NULL, NULL);
        return NULL;
    }

    if (setjmp(png_jmpbuf(pngPtr)))
    {
        delete img;
        png_destroy_read_struct(&pngPtr, &infoPtr, NULL);
        std::fclose(fp);
        return NULL;
    }

    png_init_io(pngPtr, fp);
    png_set_sig_bytes(pngPtr, sizeof(header));

    png_read_info(pngPtr, infoPtr);
    if (png_get_IHDR(pngPtr, infoPtr, &width, &height, &bitDepth, &colorType, NULL, NULL, NULL) == 0)
        png_error(pngPtr, _("Failed to read IHDR chunk"));

    if (width == 0 || width > Image::MAX_IMAGE_DIMENSION)
        png_error(pngPtr, _("Image width out of range"));
    if (height == 0 || height > Image::MAX_IMAGE_DIMENSION)
        png_error(pngPtr, _("Image height out of range"));

    glFormat = PNGGetGLFormat(pngPtr, infoPtr, bitDepth, colorType);

    img = new Image(glFormat, width, height);
    if (!img)
        png_error(pngPtr, _("Failed to create image object"));
    pitch = img->getPitch();

    passes = png_set_interlace_handling(pngPtr);
    for (int pass = 0; pass < passes; ++pass)
    {
        png_bytep rowPtr = img->getPixels();
        for (png_uint_32 row = 0; row < height; ++row)
        {
            png_read_row(pngPtr, rowPtr, NULL);
            rowPtr += pitch;
        }
    }

    png_read_end(pngPtr, NULL);
    png_destroy_read_struct(&pngPtr, &infoPtr, NULL);

    std::fclose(fp);

    return img;
}


// BMP file definitions--can't use windows.h because we might not be
// built on Windows!
typedef struct
{
    char magic[2];
    uint32 size;
    uint32 reserved;
    uint32 offset;
} BMPFileHeader;

typedef struct
{
    uint32 size;
    int32 width;
    int32 height;
    uint16 planes;
    uint16 bpp;
    uint32 compression;
    uint32 imageSize;
    int32 widthPPM;
    int32 heightPPM;
    uint32 colorsUsed;
    uint32 colorsImportant;
} BMPImageHeader;


static bool readInteger(ifstream& in, int32& ret)
{
    unsigned char b[4];
    if (!in.read(reinterpret_cast<char*>(b), 4))
        return false;

    ret = (int32)(((uint32)b[3] << 24U) + ((uint32)b[2] << 16U)
        + ((uint32)b[1] << 8U) + (uint32)b[0]);
    return true;
}

static bool readInteger(ifstream& in, uint32& ret)
{
    unsigned char b[4];
    if (!in.read(reinterpret_cast<char*>(b), 4))
        return false;

    ret = ((uint32)b[3] << 24U) + ((uint32)b[2] << 16U)
        + ((uint32)b[1] << 8U) + (uint32)b[0];
    return true;
}

static bool readInteger(ifstream& in, uint16& ret)
{
    unsigned char b[2];
    if (!in.read(reinterpret_cast<char*>(b), 2))
        return false;

    ret = (uint16)(((int)b[1] << 8) + (int)b[0]);
    return true;
}


static Image* LoadBMPImage(ifstream& in)
{
    BMPFileHeader fileHeader;
    BMPImageHeader imageHeader;
    unsigned char* pixels;

    if (!in.read(fileHeader.magic, 2) || fileHeader.magic[0] != 'B' || fileHeader.magic[1] != 'M')
        return NULL;
    if (!readInteger(in, fileHeader.size) ||
        !in.seekg(4, ios_base::cur) || // skip reserved
        !readInteger(in, fileHeader.offset))
    {
        return NULL;
    }

    if (!readInteger(in, imageHeader.size) ||
        // We currently don't support legacy BMP formats
        (imageHeader.size != 40 && imageHeader.size != 52 && imageHeader.size != 56 &&
         imageHeader.size != 108 && imageHeader.size != 124))
    {
        return NULL;
    }

    if (!readInteger(in, imageHeader.width) ||
        imageHeader.width <= 0 || imageHeader.width > Image::MAX_IMAGE_DIMENSION)
    {
        return NULL;
    }
    if (!readInteger(in, imageHeader.height) ||
        imageHeader.height <= 0 || imageHeader.height > Image::MAX_IMAGE_DIMENSION)
    {
        return NULL;
    }
    if (!readInteger(in, imageHeader.planes) || imageHeader.planes != 1)
        return NULL;
    if (!readInteger(in, imageHeader.bpp) ||
        // We currently don't support 1, 2, 4, or 16-bit images
        (imageHeader.bpp != 8 && imageHeader.bpp != 24 && imageHeader.bpp != 32))
    {
        return NULL;
    }
    if (!readInteger(in, imageHeader.compression) ||
        // We currently don't support compressed BMPs
        imageHeader.compression != 0)
    {
        return NULL;
    }
    if (!readInteger(in, imageHeader.imageSize))
        return NULL;

    unsigned char* palette = NULL;
    if (imageHeader.bpp == 8)
    {
        if (!in.seekg(8, ios_base::cur) || // skip widthPPM, heightPPM
            !readInteger(in, imageHeader.colorsUsed) ||
            imageHeader.colorsUsed > 256)
        {
            return NULL;
        }

        if (imageHeader.colorsUsed == 0)
            imageHeader.colorsUsed = 256;

        if (!in.seekg(14U + imageHeader.size, ios_base::beg))
            return NULL;

        palette = new unsigned char[256 * 4];
        if (palette == NULL)
            return NULL;

        if (!in.read(reinterpret_cast<char*>(palette), 4U * imageHeader.colorsUsed))
            return NULL;

        // Fill unused palette entries with magenta
        unsigned char* palettePtr = palette + 4U * imageHeader.colorsUsed;
        for (uint32 i = imageHeader.colorsUsed; i < 256U; ++i)
        {
            palettePtr[0] = 255;
            palettePtr[1] = 0;
            palettePtr[2] = 255;
            palettePtr[3] = 0;
            palettePtr += 4;
        }
    }

    if (!in.seekg(fileHeader.offset, ios::beg))
    {
        delete[] palette;
        return NULL;
    }

    unsigned int bytesPerRow = ((imageHeader.width * imageHeader.bpp / 8U) + 3U) & (~3U);
    unsigned int imageBytes = bytesPerRow * imageHeader.height;

    // slurp the image data
    pixels = new unsigned char[imageBytes];
    if (!in.read(reinterpret_cast<char*>(pixels), imageBytes))
    {
        delete[] pixels;
        delete[] palette;
        return NULL;
    }

    // check for truncated file

    Image* img = new Image(GL_RGB, imageHeader.width, imageHeader.height);
    if (img == NULL)
    {
        delete[] pixels;
        delete[] palette;
        return NULL;
    }

    // Copy the image and perform any necessary conversions
    const unsigned char* srcRow = pixels;
    for (int32 y = imageHeader.height; y-- > 0;)
    {
        const unsigned char* src = srcRow;
        unsigned char* dst = img->getPixelRow(y);

        switch (imageHeader.bpp)
        {
        case 8:
            {
                for (int32 x = 0; x < imageHeader.width; x++)
                {
                    const unsigned char* color = palette + (*src * 4);
                    dst[0] = color[2];
                    dst[1] = color[1];
                    dst[2] = color[0];
                    src++;
                    dst += 3;
                }
            }
            break;

        case 24:
            {
                for (int32 x = 0; x < imageHeader.width; x++)
                {
                    dst[0] = src[2];
                    dst[1] = src[1];
                    dst[2] = src[0];
                    src += 3;
                    dst += 3;
                }
            }
            break;

        case 32:
            {
                for (int32 x = 0; x < imageHeader.width; x++)
                {
                    dst[0] = src[2];
                    dst[1] = src[1];
                    dst[2] = src[0];
                    src += 4;
                    dst += 3;
                }
            }
            break;
        }

        srcRow += bytesPerRow;
    }

    delete[] pixels;
    delete[] palette;

    return img;
}


Image* LoadBMPImage(const string& filename)
{
    ifstream bmpFile(filename.c_str(), ios::in | ios::binary);

    if (bmpFile.good())
    {
        Image* img = LoadBMPImage(bmpFile);
        bmpFile.close();
        return img;
    }
    else
    {
        return NULL;
    }
}
