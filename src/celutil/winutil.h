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

const char* CurrentCP();
std::string UTF8ToCurrentCP(const std::string& str);
std::string CurrentCPToUTF8(const std::string& str);
std::string WideToCurrentCP(const std::wstring& ws);
std::wstring CurrentCPToWide(const std::string& str);
std::string WideToUTF8(const std::wstring& ws);
std::wstring UTF8ToWide(const std::string& str);
