// wstringutils.h
//
// Copyright (C) 2023, Celestia Development Team
//
// String conversions to/from Windows character sets
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

#include <fmt/format.h>

#include <windows.h>

namespace celestia::win32
{

// UTF-8 to wchar_t, growable
template<typename T, std::enable_if_t<std::is_same_v<typename T::value_type, wchar_t>, int> = 0>
int
AppendUTF8ToWide(std::string_view source, T& destination)
{
    if (source.empty())
        return 0;

    const auto sourceSize = static_cast<int>(source.size());
    int wideLength = MultiByteToWideChar(CP_UTF8, 0, source.data(), sourceSize, nullptr, 0);
    if (wideLength <= 0)
        return 0;

    const auto existingSize = destination.size();
    destination.resize(existingSize + static_cast<std::size_t>(wideLength));
    wideLength = MultiByteToWideChar(CP_UTF8, 0,
                                     source.data(), sourceSize,
                                     destination.data() + existingSize, wideLength);
    if (wideLength <= 0)
    {
        destination.resize(existingSize);
        return 0;
    }

    destination.resize(existingSize + static_cast<std::size_t>(wideLength));
    return wideLength;
}

// UTF-8 to wchar_t, fixed buffer
int UTF8ToWide(std::string_view str, wchar_t* dest, int destSize);

inline std::wstring
UTF8ToWideString(std::string_view str)
{
    std::wstring result;
    AppendUTF8ToWide(str, result);
    return result;
}

template<typename T, std::enable_if_t<std::is_same_v<typename T::value_type, char>, int> = 0>
int
AppendWideToUTF8(std::wstring_view source, T& destination)
{
    if (source.empty())
        return 0;

    const auto sourceSize = static_cast<int>(source.size());
    const auto existingSize = destination.size();
    int length = WideCharToMultiByte(CP_UTF8, 0, source.data(), sourceSize, nullptr, 0, nullptr, nullptr);
    if (length <= 0)
        return 0;

    destination.resize(existingSize + static_cast<std::size_t>(length));
    length = WideCharToMultiByte(CP_UTF8, 0,
                                 source.data(), sourceSize,
                                 destination.data() + existingSize, length,
                                 nullptr, nullptr);
    if (length <= 0)
    {
        destination.resize(existingSize);
        return 0;
    }

    destination.resize(existingSize + static_cast<std::size_t>(length));
    return length;
}

inline std::string
WideToUTF8String(std::wstring_view tstr)
{
    std::string result;
    AppendWideToUTF8(tstr, result);
    return result;
}

int CompareUTF8Localized(std::string_view, std::string_view);

} // end namespace celestia::win32
