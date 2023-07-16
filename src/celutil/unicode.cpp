// unicode.cpp
//
// Copyright (C) 2023-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "unicode.h"

#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>

#ifdef HAVE_WIN_ICU_COMBINED_HEADER
#include <icu.h>
#elif defined(HAVE_WIN_ICU_SEPARATE_HEADERS)
#include <icucommon.h>
#include <icui18n.h>
#else
#include <unicode/ubidi.h>
#include <unicode/umachine.h>
#include <unicode/ushape.h>
#include <unicode/ustring.h>
#endif

#include "flag.h"


static_assert(std::is_same_v<UChar, char16_t>);

namespace celestia::util
{

namespace
{

bool
ApplyArabicShaping(std::u16string_view input, std::u16string &output)
{
    UErrorCode error = U_ZERO_ERROR;
    std::uint32_t options = U_SHAPE_LETTERS_SHAPE | U_SHAPE_TEXT_DIRECTION_LOGICAL | U_SHAPE_LENGTH_GROW_SHRINK;
    auto inputSize = static_cast<std::int32_t>(input.size());
    auto requiredSize = u_shapeArabic(input.data(), inputSize, nullptr, 0, options, &error);
    if (U_FAILURE(error) && error != U_BUFFER_OVERFLOW_ERROR)
        return false;

    // Clear the error to be reused
    error = U_ZERO_ERROR;

    output.resize(requiredSize);
    u_shapeArabic(input.data(), inputSize, output.data(), requiredSize, options, &error);
    return U_SUCCESS(error);
}

bool
ApplyBidiReordering(std::u16string_view input, std::u16string &output)
{
    auto ubidi = ubidi_open();
    UErrorCode error = U_ZERO_ERROR;
    auto inputSize = static_cast<std::int32_t>(input.size());
    ubidi_setPara(ubidi, input.data(), inputSize, UBIDI_DEFAULT_LTR, nullptr, &error);

    std::uint16_t options = UBIDI_DO_MIRRORING | UBIDI_REMOVE_BIDI_CONTROLS;

    auto requiredSize = ubidi_writeReordered(ubidi, nullptr, 0, options, &error);
    if (U_FAILURE(error) && error != U_BUFFER_OVERFLOW_ERROR)
    {
        ubidi_close(ubidi);
        return false;
    }

    output.resize(requiredSize);

    // Clear the error to be reused
    error = U_ZERO_ERROR;

    ubidi_writeReordered(ubidi, output.data(), requiredSize, options, &error);
    ubidi_close(ubidi);
    return U_SUCCESS(error);
}

} // end unnamed namespace


bool
UTF8StringToUnicodeString(std::string_view input, std::u16string &output)
{
    if (input.empty())
    {
        output.clear();
        return true;
    }

    std::int32_t requiredSize;
    UErrorCode error = U_ZERO_ERROR;

    // dry run to get the required size for unicode string
    auto inputLength = static_cast<std::int32_t>(input.size());
    u_strFromUTF8(nullptr, 0, &requiredSize, input.data(), inputLength, &error);
    if (U_FAILURE(error) && error != U_BUFFER_OVERFLOW_ERROR)
        return false;

    output.resize(requiredSize);

    // Clear the error to be reused
    error = U_ZERO_ERROR;

    // Get the actual data
    u_strFromUTF8(output.data(), requiredSize, nullptr, input.data(), inputLength, &error);
    return U_SUCCESS(error);
}


bool
ApplyBidiAndShaping(std::u16string_view input, std::u16string &output, ConversionOption options)
{
    output.clear();
    if (input.empty())
        return true;

    output.append(input);
    if (is_set(options, ConversionOption::ArabicShaping))
    {
        std::u16string result;
        if (ApplyArabicShaping(output, result))
            output = std::move(result);
        else
            return false;
    }

    if (is_set(options, ConversionOption::BidiReordering))
    {
        std::u16string result;
        if (ApplyBidiReordering(output, result))
            output = std::move(result);
        else
            return false;
    }

    return true;
}

}
