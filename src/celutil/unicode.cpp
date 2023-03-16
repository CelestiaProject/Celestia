// unicode.cpp
//
// Copyright (C) 2023-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "unicode.h"

#include <unicode/ustring.h>

namespace celestia::util
{

bool UnicodeStringToWString(const icu::UnicodeString &input, std::wstring &output)
{
    if (input.length() == 0)
    {
        output.resize(1);
        output[0] = L'\0';
        return true;
    }

    int32_t requiredSize;
    UErrorCode error = U_ZERO_ERROR;

    // dry run to get the required size for wstring
    u_strToWCS(nullptr, 0, &requiredSize, input.getBuffer(), input.length(), &error);
    if (error != U_BUFFER_OVERFLOW_ERROR)
        return false;

    output.resize(requiredSize + 1);

    // Clear the error to be reused
    error = U_ZERO_ERROR;

    // Get the actual data
    u_strToWCS(output.data(), static_cast<int32_t>(output.size()), nullptr, input.getBuffer(), input.length(), &error);

    // Removing the redundant \0 at the end
    output.resize(requiredSize);
    return error == U_ZERO_ERROR;
}

}
