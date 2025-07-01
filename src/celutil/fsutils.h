// fsutils.h
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@shatters.net>
//
// Miscellaneous useful filesystem-related functions.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string_view>

#include <celutil/array_view.h>

namespace celestia::util
{

// Since std::hash<std::filesystem::path> was not in the original C++17 standard
// we need to implement a custom hasher for older compilers.
struct PathHasher
{
    std::size_t operator()(const std::filesystem::path& path) const noexcept
    {
        return std::filesystem::hash_value(path);
    }
};

std::optional<std::filesystem::path> U8FileName(std::string_view source,
                                   bool allowWildcardExtension = true);
std::filesystem::path LocaleFilename(const std::filesystem::path& filename);
std::filesystem::path PathExp(std::filesystem::path&& filename);
std::filesystem::path ResolveWildcard(const std::filesystem::path& wildcard,
                         array_view<std::string_view> extensions);
bool IsValidDirectory(const std::filesystem::path &dir);
#ifndef PORTABLE_BUILD
std::filesystem::path HomeDir();
std::filesystem::path WriteableDataPath();
#endif

} // end namespace celestia::util
