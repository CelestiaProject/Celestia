// winutil.cpp
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

#include "winutil.h"

#include <windows.h>

namespace celestia::util::detail
{

int WideToUTF8(std::wstring_view ws, char* out, int outSize)
{
    return WideCharToMultiByte(CP_UTF8, 0,
                               ws.data(), static_cast<int>(ws.size()),
                               out, outSize,
                               nullptr, nullptr);
}

} // end namespace celestia::util::detail
