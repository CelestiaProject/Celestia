// unicode.cpp
//
// Copyright (C) 2023-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <vector>
#include <unicode/ubidi.h>
#include <unicode/ushape.h>
#include <unicode/ustring.h>

#include "flag.h"
#include "unicode.h"

namespace celestia::util
{

bool ApplyArabicShaping(const std::vector<UChar> &input, std::vector<UChar> &output)
{
    UErrorCode error = U_ZERO_ERROR;
    uint32_t options = U_SHAPE_LETTERS_SHAPE | U_SHAPE_TEXT_DIRECTION_LOGICAL | U_SHAPE_LENGTH_GROW_SHRINK;
    auto requiredSize = u_shapeArabic(input.data(), static_cast<int32_t>(input.size()), nullptr, 0, options, &error);
    if (error != U_BUFFER_OVERFLOW_ERROR)
        return false;

    // Clear the error to be reused
    error = U_ZERO_ERROR;

    output.resize(requiredSize + 1);
    u_shapeArabic(input.data(), static_cast<int32_t>(input.size()), output.data(), static_cast<int32_t>(output.size()), options, &error);
    output.resize(requiredSize);
    return error == U_ZERO_ERROR;
}

bool ApplyBidiReordering(const std::vector<UChar> &input, std::vector<UChar> &output)
{
    auto ubidi = ubidi_open();
    UErrorCode error = U_ZERO_ERROR;
    ubidi_setPara(ubidi, input.data(), static_cast<int32_t>(input.size()), UBIDI_DEFAULT_LTR, nullptr, &error);

    uint16_t options = UBIDI_DO_MIRRORING | UBIDI_REMOVE_BIDI_CONTROLS;

    auto requiredSize = ubidi_writeReordered(ubidi, nullptr, 0, options, &error);
    if (error != U_BUFFER_OVERFLOW_ERROR)
    {
        ubidi_close(ubidi);
        return false;
    }

    // Clear the error to be reused
    error = U_ZERO_ERROR;

    output.resize(requiredSize + 1);
    ubidi_writeReordered(ubidi, output.data(), static_cast<int32_t>(output.size()), options, &error);
    ubidi_close(ubidi);
    output.resize(requiredSize);
    return error == U_ZERO_ERROR;
}

bool UTF8StringToUnicodeString(std::string_view input, std::vector<UChar> &output)
{
    if (input.empty())
    {
        output.clear();
        return true;
    }

    int32_t requiredSize;
    UErrorCode error = U_ZERO_ERROR;

    // dry run to get the required size for unicode string
    u_strFromUTF8(nullptr, 0, &requiredSize, input.data(), static_cast<int32_t>(input.size()), &error);
    if (error != U_BUFFER_OVERFLOW_ERROR)
        return false;

    output.resize(requiredSize + 1);

    // Clear the error to be reused
    error = U_ZERO_ERROR;

    // Get the actual data
    u_strFromUTF8(output.data(), static_cast<int32_t>(output.size()), nullptr, input.data(), static_cast<int32_t>(input.size()), &error);

    // Removing the redundant \0 at the end
    output.resize(requiredSize);
    return error == U_ZERO_ERROR;
}

bool UnicodeStringToWString(const std::vector<UChar> &input, std::wstring &output, ConversionOption options)
{
    if (input.empty())
    {
        output.clear();
        return true;
    }

    auto current = input;
    if (is_set(options, ConversionOption::ArabicShaping))
    {
        std::vector<UChar> result;
        if (ApplyArabicShaping(current, result))
            current.swap(result);
        else
            return false;
    }

    if (is_set(options, ConversionOption::BidiReordering))
    {
        std::vector<UChar> result;
        if (ApplyBidiReordering(current, result))
            current.swap(result);
        else
            return false;
    }

    int32_t requiredSize;
    UErrorCode error = U_ZERO_ERROR;

    // dry run to get the required size for wstring
    u_strToWCS(nullptr, 0, &requiredSize, current.data(), static_cast<int32_t>(current.size()), &error);
    if (error != U_BUFFER_OVERFLOW_ERROR)
        return false;

    output.resize(requiredSize + 1);

    // Clear the error to be reused
    error = U_ZERO_ERROR;

    // Get the actual data
    u_strToWCS(output.data(), static_cast<int32_t>(output.size()), nullptr, current.data(), static_cast<int32_t>(current.size()), &error);

    // Removing the redundant \0 at the end
    output.resize(requiredSize);
    return error == U_ZERO_ERROR;
}

}
