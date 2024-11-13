// avif.cpp
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <limits>
#include <memory>
#include <vector>

#include <celcompat/filesystem.h>
#include <celimage/image.h>
#include <celutil/logger.h>
#include <celutil/uniquedel.h>

extern "C"
{
#include <avif/avif.h>
}

namespace celestia::engine
{

namespace
{

using UniqueAVIFDecoder = util::UniquePtrDel<avifDecoder, avifDecoderDestroy>;

// We implement our own AVIF solution because libavif does not provide a way
// to open a file with a Windows-style wchar_t-based path.

struct FStreamAVIFIO
{
    avifIO io; // First member to ensure pointer interconvertibility
    std::ifstream file;
    std::vector<std::uint8_t> buffer;
};

avifResult
FStreamAVIFIORead(avifIO* io, std::uint32_t readFlags, std::uint64_t offset, std::size_t size, avifROData* out)
{
    if (readFlags != 0)
        return AVIF_RESULT_IO_ERROR;

    auto reader = reinterpret_cast<FStreamAVIFIO*>(io);
    if (offset > reader->io.sizeHint)
        return AVIF_RESULT_IO_ERROR;

    if (std::uint64_t availableSize = reader->io.sizeHint - offset; size > availableSize)
        size = static_cast<std::size_t>(availableSize);

    if (size > std::numeric_limits<std::streamsize>::max())
        size = static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max());

    if (size > 0)
    {
        if (offset > std::numeric_limits<std::streamoff>::max())
            return AVIF_RESULT_IO_ERROR;

        if (!reader->file.seekg(static_cast<std::streamoff>(offset), std::ios_base::beg))
            return AVIF_RESULT_IO_ERROR;

        reader->buffer.resize(size);

        if (!reader->file.read(reinterpret_cast<char*>(reader->buffer.data()), static_cast<std::streamsize>(size)) && /* Flawfinder: ignore */
            !reader->file.eof())
        {
            return AVIF_RESULT_IO_ERROR;
        }

        size = static_cast<std::size_t>(reader->file.gcount());
    }

    out->data = reader->buffer.data();
    out->size = size;

    return AVIF_RESULT_OK;
}

} // end unnamed namespace

Image*
LoadAVIFImage(const fs::path& filename)
{
    UniqueAVIFDecoder decoder{ avifDecoderCreate() };
    if (!decoder)
        return nullptr;

    FStreamAVIFIO reader;
    reader.file.open(filename, std::ios_base::in | std::ios_base::binary);
    if (!reader.file)
    {
        util::GetLogger()->error("Cannot open file for read: '{}'\n", filename);
        return nullptr;
    }

    reader.io.read = &FStreamAVIFIORead;
    reader.io.write = nullptr; // unused
    reader.io.destroy = nullptr; // automatic lifetime ends on scope exit
    reader.io.sizeHint = static_cast<std::uint64_t>(fs::file_size(filename));
    reader.io.persistent = AVIF_FALSE;
    reader.io.data = nullptr; // unused

    avifDecoderSetIO(decoder.get(), reinterpret_cast<avifIO*>(&reader));

    if (avifResult result = avifDecoderParse(decoder.get()); result != AVIF_RESULT_OK)
    {
        util::GetLogger()->error("Failed to decode image: {}\n", avifResultToString(result));
        return nullptr;
    }

    if (avifDecoderNextImage(decoder.get()) != AVIF_RESULT_OK)
    {
        util::GetLogger()->error("No image available: {}\n", filename);
        return nullptr;
    }

    avifRGBImage rgb;
    rgb.format = AVIF_RGB_FORMAT_RGBA;
    avifRGBImageSetDefaults(&rgb, decoder->image);
    if (rgb.width > Image::MAX_DIMENSION || rgb.height > Image::MAX_DIMENSION)
    {
        util::GetLogger()->error("Image exceeds maximum dimensions: {}\n", filename);
        return nullptr;
    }

    auto image = std::make_unique<Image>(PixelFormat::RGBA, rgb.width, rgb.height);
    rgb.pixels = image->getPixels();
    rgb.rowBytes = image->getWidth() * image->getComponents();

    if (avifImageYUVToRGB(decoder->image, &rgb) != AVIF_RESULT_OK)
    {
        util::GetLogger()->error("Conversion from YUV failed: {}\n", filename);
        return nullptr;
    }

    return image.release();
}

} // namespace celestia::engine
