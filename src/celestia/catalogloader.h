// catalogloader.cpp
//
// Copyright (C) 2001-2023, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <iosfwd>
#include <filesystem>
#include <string_view>

#include <celutil/array_view.h>
#include <celutil/filetype.h>

class ProgressNotifier;

namespace celestia
{
namespace engine
{
class GeometryPaths;
class TexturePaths;
}

class CatalogLoader
{
public:
    virtual ~CatalogLoader() = default;

    virtual bool load(std::istream &in, const std::filesystem::path &dir) = 0;

    void process(const std::filesystem::path &filePath, const std::filesystem::path &parentPath);
    void loadExtras(util::array_view<std::filesystem::path> dirs);

protected:
    CatalogLoader(ProgressNotifier* notifier,
                  util::array_view<std::filesystem::path> skipPaths);

    virtual ContentType contentType() const = 0;
    virtual std::string_view typeDesc() const = 0;

private:
    ProgressNotifier* m_notifier;
    util::array_view<std::filesystem::path> m_skipPaths;
};

} // namespace celestia
