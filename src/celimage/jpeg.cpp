// jpeg.cpp
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstdio>  // fopen, fclose
#include <cstring> // memcpy
#include <setjmp.h>
extern "C" {
#include <jpeglib.h>
}
#include <celengine/image.h>
#include <celutil/debug.h>

using celestia::PixelFormat;

namespace
{
struct my_error_mgr
{
    struct jpeg_error_mgr pub;  // "public" fields
    jmp_buf setjmp_buffer;      // for return to caller
};

using my_error_ptr = struct my_error_mgr *;

METHODDEF(void) my_error_exit(j_common_ptr cinfo)
{
    // cinfo->err really points to a my_error_mgr struct, so coerce pointer
    auto myerr = (my_error_ptr) cinfo->err;

    // Always display the message.
    // We could postpone this until after returning, if we chose.
    (*cinfo->err->output_message) (cinfo);

    // Return control to the setjmp point
    longjmp(myerr->setjmp_buffer, 1);
}
} // anonymous namespace

Image* LoadJPEGImage(const fs::path& filename, int /*unused*/)
{
    Image* img = nullptr;

    // This struct contains the JPEG decompression parameters and pointers to
    // working space (which is allocated as needed by the JPEG library).
    struct jpeg_decompress_struct cinfo;

    // We use our private extension JPEG error handler.
    // Note that this struct must live as long as the main JPEG parameter
    // struct, to avoid dangling-pointer problems.
    struct my_error_mgr jerr;
    // More stuff
    JSAMPARRAY buffer;        // Output row buffer
    int row_stride;        // physical row width in output buffer
    long cont;

    // In this example we want to open the input file before doing anything else,
    // so that the setjmp() error recovery below can assume the file is open.
    // VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
    // requires it in order to read binary files.

    FILE *in;
#ifdef _WIN32
    in = _wfopen(filename.c_str(), L"rb");
#else
    in = fopen(filename.c_str(), "rb");
#endif
    if (!in)
        return nullptr;

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
        delete img;

        return nullptr;
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

    PixelFormat format = PixelFormat::RGB;
    if (cinfo.output_components == 1)
        format = PixelFormat::LUMINANCE;

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

bool SaveJPEGImage(const fs::path& filename,
                   int width, int height,
                   int rowStride,
                   unsigned char *pixels,
                   bool removeAlpha)
{
    FILE* out;
#ifdef _WIN32
    out = _wfopen(filename.c_str(), L"wb");
#else
    out = fopen(filename.c_str(), "wb");
#endif
    if (out == nullptr)
    {
        DPRINTF(LOG_LEVEL_ERROR, "Can't open screen capture file '%s'\n", filename);
        return false;
    }

    struct jpeg_compress_struct cinfo;

    struct jpeg_error_mgr jerr;
    JSAMPROW row[1];

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    jpeg_stdio_dest(&cinfo, out);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);

    jpeg_set_quality(&cinfo, 90, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    while (cinfo.next_scanline < cinfo.image_height)
    {
        unsigned char *rowHead = &pixels[rowStride * cinfo.next_scanline];
        // Strip alpha values if we are in RGBA format
        if (removeAlpha)
        {
            for (int x = 0; x < width; x++)
            {
                const unsigned char* pixelIn = &rowHead[x * 4];
                unsigned char* pixelOut = &rowHead[x * 3];
                pixelOut[0] = pixelIn[0];
                pixelOut[1] = pixelIn[1];
                pixelOut[2] = pixelIn[2];
            }
        }
        row[0] = rowHead;
        (void) jpeg_write_scanlines(&cinfo, row, 1);
    }

    jpeg_finish_compress(&cinfo);
    fclose(out);
    jpeg_destroy_compress(&cinfo);

    return true;
}

bool SaveJPEGImage(const fs::path& filename, Image& image)
{
    return SaveJPEGImage(filename,
                         image.getWidth(),
                         image.getHeight(),
                         image.getPitch(),
                         image.getPixels(),
                         image.hasAlpha());
}
