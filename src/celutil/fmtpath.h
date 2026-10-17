// logger.h
//
// Copyright (C) 2026-present, Celestia Development Team
//
// Filesystem path formatter, extracted from logger.h
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <filesystem>

#if FMT_VERSION >= 100100
// From fmt 10.1.0 onwards, default path formatter is unquoted
#include <fmt/std.h>
#else
#include <string_view>

#if defined(_WIN32)
#include <celutil/winutil.h>
#endif

template <>
struct fmt::formatter<std::filesystem::path> : formatter<std::string_view>
{
    auto format(const std::filesystem::path &path, format_context &ctx) const
    {
#ifdef _WIN32
        fmt::basic_memory_buffer<char> buffer;
        celestia::util::WideToUTF8(path.native(), buffer);
        return formatter<std::string_view>::format(std::string_view{ buffer.data(), buffer.size() }, ctx);
#else
        return formatter<std::string_view>::format(path.native(), ctx);
#endif
    }
};
#endif
