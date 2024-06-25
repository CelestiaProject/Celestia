// tstring.h
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

using tstring = std::basic_string<TCHAR>;
using tstring_view = std::basic_string_view<TCHAR>;

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

template<typename T, std::enable_if_t<std::is_same_v<typename T::value_type, wchar_t>, int> = 0>
int
AppendCurrentCPToWide(std::string_view source, T& destination)
{
    if (source.empty())
        return 0;

    const auto sourceSize = static_cast<int>(source.size());
    int wideLength = MultiByteToWideChar(CP_ACP, 0, source.data(), sourceSize, nullptr, 0);
    if (wideLength <= 0)
        return 0;

    const auto existingSize = destination.size();
    destination.resize(existingSize + static_cast<std::size_t>(wideLength));
    wideLength = MultiByteToWideChar(CP_ACP, 0,
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

template<typename T, std::enable_if_t<std::is_same_v<typename T::value_type, char>, int> = 0>
int
AppendWideToCurrentCP(std::wstring_view source, T& destination)
{
    if (source.empty())
        return 0;

    const auto sourceSize = static_cast<int>(source.size());
    int destLength = WideCharToMultiByte(CP_ACP, 0, source.data(), sourceSize, nullptr, 0, nullptr, nullptr);
    if (destLength <= 0)
        return 0;

    const auto existingSize = destination.size();
    destination.resize(existingSize + static_cast<std::size_t>(destLength));
    destLength = WideCharToMultiByte(CP_ACP, 0,
                                     source.data(), sourceSize,
                                     destination.data() + existingSize, destLength,
                                     nullptr, nullptr);
    if (destLength <= 0)
    {
        destination.resize(existingSize);
        return 0;
    }

    destination.resize(existingSize + static_cast<std::size_t>(destLength));
    return destLength;
}

inline std::string
WideToCurrentCP(std::wstring_view wstr)
{
    std::string result;
    AppendWideToCurrentCP(wstr, result);
    return result;
}

// UTF-8 to TCHAR, fixed buffer
int UTF8ToTChar(std::string_view str, tstring::value_type* dest, int destSize);

// UTF-8 to TCHAR, growable
template<typename T, std::enable_if_t<std::is_same_v<typename T::value_type, TCHAR>, int> = 0>
int
AppendUTF8ToTChar(std::string_view source, T& destination)
{
#ifdef _UNICODE
    return AppendUTF8ToWide(source, destination);
#else
    fmt::basic_memory_buffer<wchar_t> wbuffer;
    if (AppendUTF8ToWide(source, wbuffer) <= 0)
        return 0;

    return AppendWideToCurrentCP(std::wstring_view(wbuffer.data(), wbuffer.size()), destination);
#endif
}

inline tstring
UTF8ToTString(std::string_view str)
{
    tstring result;
    AppendUTF8ToTChar(str, result);
    return result;
}

template<typename T, std::enable_if_t<std::is_same_v<typename T::value_type, char>, int> = 0>
int
AppendTCharToUTF8(tstring_view source, T& destination)
{
    if (source.empty())
        return 0;

    const auto sourceSize = static_cast<int>(source.size());
    const auto existingSize = destination.size();
#ifdef _UNICODE
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
#else
    int wlength = MultiByteToWideChar(CP_ACP, 0, source.data(), sourceSize, nullptr, 0);
    if (wlength <= 0)
        return 0;

    fmt::basic_memory_buffer<wchar_t> wbuffer;
    wbuffer.resize(static_cast<std::size_t>(wlength));
    wlength = MultiByteToWideChar(CP_ACP, 0, source.data(), sourceSize, wbuffer.data(), wlength);
    if (wlength <= 0)
        return 0;
    wbuffer.resize(static_cast<std::size_t>(wlength));

    int length = WideCharToMultiByte(CP_UTF8, 0, wbuffer.data(), wlength, nullptr, 0, nullptr, nullptr);
    if (length <= 0)
        return 0;

    destination.resize(existingSize + static_cast<std::size_t>(length));
    length = WideCharToMultiByte(CP_UTF8, 0,
                                 wbuffer.data(), wlength,
                                 destination.data() + existingSize, length,
                                 nullptr, nullptr);
    if (length <= 0)
    {
        destination.resize(existingSize);
        return 0;
    }

    destination.resize(existingSize + static_cast<std::size_t>(length));
#endif
    return length;
}

inline std::string
TCharToUTF8String(tstring_view tstr)
{
    std::string result;
    AppendTCharToUTF8(tstr, result);
    return result;
}

int CompareUTF8Localized(std::string_view, std::string_view);

} // end namespace celestia::win32
