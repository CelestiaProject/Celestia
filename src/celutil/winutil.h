// winutil.h
//
// Copyright (C) 2019-present, Celestia Development Team
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// Miscellaneous useful Windows-related functions.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string>
#include <string_view>

namespace celestia::util
{

namespace detail
{
int WideToUTF8(std::wstring_view ws, char* out, int outSize);
}

template<typename T>
void WideToUTF8(std::wstring_view ws, T& output)
{
    if (ws.empty())
        return;

    // get a converted string length
    const auto srcLen = static_cast<int>(ws.size());
    int len = detail::WideToUTF8(ws, nullptr, 0);
    if (len <= 0)
        return;

    using size_type = decltype(output.size());
    size_type initialSize = output.size();
    auto newSize = static_cast<size_type>(initialSize + len);
    output.resize(newSize);

    len = detail::WideToUTF8(ws, output.data() + initialSize, len);
    if (len <= 0)
    {
        output.resize(initialSize);
        return;
    }

    newSize = static_cast<size_type>(initialSize + len);
    output.resize(static_cast<std::size_t>(newSize));
}

inline std::string
WideToUTF8(std::wstring_view ws)
{
    std::string result;
    WideToUTF8(ws, result);
    return result;
}

}
