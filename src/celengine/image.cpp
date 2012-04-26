// image.cpp
//
// Copyright (C) 2001, Chris Laurel
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <fstream>

#ifndef TARGET_OS_MAC
#define JPEG_SUPPORT
#define PNG_SUPPORT
#endif

#ifdef TARGET_OS_MAC
#include <unistd.h>
#include "CGBuffer.h"
#ifndef PNG_SUPPORT
#include <Quicktime/ImageCompression.h>
#include <QuickTime/QuickTimeComponents.h>
#endif
#endif

#ifndef _WIN32
#ifndef TARGET_OS_MAC
#include <config.h>
#endif /* ! TARGET_OS_MAC */
#endif /* ! _WIN32 */

#include "image.h"

#ifdef JPEG_SUPPORT

#ifndef PNG_SUPPORT
#include "setjmp.h"
#endif // PNG_SUPPORT

extern "C" {
#ifdef _WIN32
#include "jpeglib.h"
#else
#include <cstdio>
#include <jpeglib.h>
#endif
}

#endif // JPEG_SUPPORT

#ifdef PNG_SUPPORT // PNG_SUPPORT
#ifdef TARGET_OS_MAC
#include "../../macosx/png.h"
#else
#include "png.h"
#endif // TARGET_OS_MAC

#include <celutil/debug.h>
#include <celutil/util.h>
#include <celutil/filetype.h>
#include <GL/glew.h>
#include "celestia.h"

#include <cassert>
#include <iostream>
#include <algorithm>
#include <cmath>

using namespace std;


// Define png_jmpbuf() in case we are using a pre-1.0.6 version of libpng
#ifndef png_jmpbuf
#define png_jmpbuf(png_ptr) png_ptr->jmpbuf
#endif // PNG_SUPPORT

// Define various expansion transformations for old versions of libpng
#if PNG_LIBPNG_VER < 10004
#define png_set_palette_to_rgb(p)  png_set_expand(p)
#define png_set_gray_1_2_4_to_8(p) png_set_expand(p)
#define png_set_tRNS_to_alpha(p)   png_set_expand(p)
#endif // PNG_LIBPNG_VER < 10004

#endif // PNG_SUPPORT


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


Image::Image(int fmt, int w, int h, int mips) :
    width(w),
    height(h),
    mipLevels(mips),
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





#ifdef JPEG_SUPPORT

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
#endif // JPEG_SUPPORT


Image* LoadJPEGImage(const string& filename, int)
{
#ifdef JPEG_SUPPORT
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

#elif defined(TARGET_OS_MAC)

    Image* img = NULL;
    CGBuffer* cgJpegImage;
    size_t img_w, img_h, img_d;

    cgJpegImage = new CGBuffer(filename.c_str());
    if (cgJpegImage == NULL) {
        char tempcwd[2048];
        getcwd(tempcwd, sizeof(tempcwd));
        DPRINTF(0, "CGBuffer :: Error opening JPEG image file %s/%s\n", tempcwd, filename.c_str());
        delete cgJpegImage;
        return NULL;
    }

    if (!cgJpegImage->LoadJPEG()) {
        char tempcwd[2048];
        getcwd(tempcwd, sizeof(tempcwd));
        DPRINTF(0, "CGBuffer :: Error loading JPEG image file %s/%s\n", tempcwd, filename.c_str());
        delete cgJpegImage;
        return NULL;
    }

    cgJpegImage->Render();

    img_w = (size_t) cgJpegImage->image_size.width;
    img_h = (size_t) cgJpegImage->image_size.height;
    img_d = (size_t) ((cgJpegImage->image_depth == 8) ? 1 : 4);

    // DPRINTF(0,"cgJpegImage :: %d x %d x %d [%d] bpp\n", img_w, img_h, (size_t)cgJpegImage->image_depth, img_d);

#ifdef MACOSX_ALPHA_JPEGS
    int format = (img_d == 1) ? GL_LUMINANCE : GL_RGBA;
#else
    int format = (img_d == 1) ? GL_LUMINANCE : GL_RGB;
#endif
    img = new Image(format, img_w, img_h);
    if (img == NULL || img->getPixels() == NULL) {
        DPRINTF(0, "Could not create image\n");
        delete cgJpegImage;
        return NULL;
    }
    // following code flips image and skips alpha byte if no alpha support
    unsigned char* bout = (unsigned char*) img->getPixels();
    unsigned char* bin  = (unsigned char*) cgJpegImage->buffer->data;
    unsigned int bcount = img_w * img_h * img_d;
    unsigned int i = 0;
    bin += bcount+(img_w*img_d); // start one row past end
    for (i=0; i<bcount; ++i)
    {
         // at end of row, move back two rows
        if ( (i % (img_w * img_d)) == 0 ) bin -= 2*(img_w * img_d);

#ifndef MACOSX_ALPHA_JPEGS
        if (( (img_d != 1) && !((i&3)^3) )) // skip extra byte
        {
            ++bin;
        } else
#endif // !MACOSX_ALPHA_JPEGS

        *bout++ = *bin++;
    }
    delete cgJpegImage;
    return img;

#else
    return NULL;
#endif // JPEG_SUPPORT
}


#ifdef PNG_SUPPORT
void PNGReadData(png_structp png_ptr, png_bytep data, png_size_t length)
{
    FILE* fp = (FILE*) png_get_io_ptr(png_ptr);
    fread((void*) data, 1, length, fp);
}
#endif


Image* LoadPNGImage(const string& filename)
{
#ifndef PNG_SUPPORT
    return NULL;
#else
    char header[8];
    png_structp png_ptr;
    png_infop info_ptr;
    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type;
    FILE* fp = NULL;
    Image* img = NULL;
    png_bytep* row_pointers = NULL;

    fp = fopen(filename.c_str(), "rb");
    if (fp == NULL)
    {
        clog << _("Error opening image file ") << filename << '\n';
        return NULL;
    }

    fread(header, 1, sizeof(header), fp);
    if (png_sig_cmp((unsigned char*) header, 0, sizeof(header)))
    {
        clog << _("Error: ") << filename << _(" is not a PNG file.\n");
        fclose(fp);
        return NULL;
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                     NULL, NULL, NULL);
    if (png_ptr == NULL)
    {
        fclose(fp);
        return NULL;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
    {
        fclose(fp);
        png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
        return NULL;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        fclose(fp);
        if (img != NULL)
            delete img;
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
        clog << _("Error reading PNG image file ") << filename << '\n';
        return NULL;
    }

    // png_init_io(png_ptr, fp);
    png_set_read_fn(png_ptr, (void*) fp, PNGReadData);
    png_set_sig_bytes(png_ptr, sizeof(header));

    png_read_info(png_ptr, info_ptr);

    png_get_IHDR(png_ptr, info_ptr,
                 &width, &height, &bit_depth,
                 &color_type, &interlace_type,
                 NULL, NULL);

    GLenum glformat = GL_RGB;
    switch (color_type)
    {
    case PNG_COLOR_TYPE_GRAY:
        glformat = GL_LUMINANCE;
        break;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        glformat = GL_LUMINANCE_ALPHA;
        break;
    case PNG_COLOR_TYPE_RGB:
        glformat = GL_RGB;
        break;
    case PNG_COLOR_TYPE_PALETTE:
    case PNG_COLOR_TYPE_RGB_ALPHA:
        glformat = GL_RGBA;
        break;
    default:
        // badness
        break;
    }

    img = new Image(glformat, width, height);
    if (img == NULL)
    {
        fclose(fp);
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
        return NULL;
    }

    // TODO: consider using paletted textures if they're available
    if (color_type == PNG_COLOR_TYPE_PALETTE)
    {
        png_set_palette_to_rgb(png_ptr);
    }

    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    {
        png_set_expand_gray_1_2_4_to_8(png_ptr);
    }

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
    {
        png_set_tRNS_to_alpha(png_ptr);
    }

    // TODO: consider passing images with < 8 bits/component to
    // GL without expanding
    if (bit_depth == 16)
        png_set_strip_16(png_ptr);
    else if (bit_depth < 8)
        png_set_packing(png_ptr);

    row_pointers = new png_bytep[height];
    for (unsigned int i = 0; i < height; i++)
        row_pointers[i] = (png_bytep) img->getPixelRow(i);

    png_read_image(png_ptr, row_pointers);

    delete[] row_pointers;

    png_read_end(png_ptr, NULL);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    fclose(fp);

    return img;
#endif
}


// BMP file definitions--can't use windows.h because we might not be
// built on Windows!
typedef struct
{
    unsigned char b;
    unsigned char m;
    unsigned int size;
    unsigned int reserved;
    unsigned int offset;
} BMPFileHeader;

typedef struct
{
    unsigned int size;
    int width;
    int height;
    unsigned short planes;
    unsigned short bpp;
    unsigned int compression;
    unsigned int imageSize;
    int widthPPM;
    int heightPPM;
    unsigned int colorsUsed;
    unsigned int colorsImportant;
} BMPImageHeader;


static int readInt(ifstream& in)
{
    unsigned char b[4];
    in.read(reinterpret_cast<char*>(b), 4);
    return ((int) b[3] << 24) + ((int) b[2] << 16)
        + ((int) b[1] << 8) + (int) b[0];
}

static short readShort(ifstream& in)
{
    unsigned char b[2];
    in.read(reinterpret_cast<char*>(b), 2);
    return ((short) b[1] << 8) + (short) b[0];
}


static Image* LoadBMPImage(ifstream& in)
{
    BMPFileHeader fileHeader;
    BMPImageHeader imageHeader;
    unsigned char* pixels;

    in >> fileHeader.b;
    in >> fileHeader.m;
    fileHeader.size = readInt(in);
    fileHeader.reserved = readInt(in);
    fileHeader.offset = readInt(in);

    if (fileHeader.b != 'B' || fileHeader.m != 'M')
        return NULL;

    imageHeader.size = readInt(in);
    imageHeader.width = readInt(in);
    imageHeader.height = readInt(in);
    imageHeader.planes = readShort(in);
    imageHeader.bpp = readShort(in);
    imageHeader.compression = readInt(in);
    imageHeader.imageSize = readInt(in);
    imageHeader.widthPPM = readInt(in);
    imageHeader.heightPPM = readInt(in);
    imageHeader.colorsUsed = readInt(in);
    imageHeader.colorsImportant = readInt(in);

    if (imageHeader.width <= 0 || imageHeader.height <= 0)
        return NULL;

    // We currently don't support compressed BMPs
    if (imageHeader.compression != 0)
        return NULL;
    // We don't handle 1-, 2-, or 4-bpp images
    if (imageHeader.bpp != 8 && imageHeader.bpp != 24 && imageHeader.bpp != 32)
        return NULL;

    unsigned char* palette = NULL;
    if (imageHeader.bpp == 8)
    {
        printf("Reading %d color palette\n", imageHeader.colorsUsed);
        palette = new unsigned char[imageHeader.colorsUsed * 4];
        in.read(reinterpret_cast<char*>(palette), imageHeader.colorsUsed * 4);
    }

    in.seekg(fileHeader.offset, ios::beg);

    unsigned int bytesPerRow =
        (imageHeader.width * imageHeader.bpp / 8 + 1) & ~1;
    unsigned int imageBytes = bytesPerRow * imageHeader.height;

    // slurp the image data
    pixels = new unsigned char[imageBytes];
    in.read(reinterpret_cast<char*>(pixels), imageBytes);

    // check for truncated file

    Image* img = new Image(GL_RGB, imageHeader.width, imageHeader.height);
    if (img == NULL)
    {
        delete[] pixels;
        return NULL;
    }

    // Copy the image and perform any necessary conversions
    for (int y = 0; y < imageHeader.height; y++)
    {
        unsigned char* src = &pixels[y * bytesPerRow];
        unsigned char* dst = img->getPixelRow(y);

        switch (imageHeader.bpp)
        {
        case 8:
            {
                for (int x = 0; x < imageHeader.width; x++)
                {
                    unsigned char* color = palette + (*src << 2);
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
                for (int x = 0; x < imageHeader.width; x++)
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
                for (int x = 0; x < imageHeader.width; x++)
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
    }

    delete[] pixels;

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
