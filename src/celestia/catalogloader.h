// catalogloader.h
//
// Copyright (C) 2001-2023, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <algorithm>
#include <fstream>
#include <string>

#include <celestia/progressnotifier.h>
#include <celutil/array_view.h>
#include <celutil/filetype.h>
#include <celutil/fsutils.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>

namespace celestia
{
namespace engine
{
class GeometryPaths;
}

template<class OBJDB> class CatalogLoader
{
public:
    CatalogLoader(OBJDB* db,
                  const std::string& typeDesc,
                  const ContentType& contentType,
                  ProgressNotifier* notifier,
                  util::array_view<std::filesystem::path> skipPaths,
                  engine::GeometryPaths& geometryPaths) :
        m_objDB(db),
        m_typeDesc(typeDesc),
        m_contentType(contentType),
        m_notifier(notifier),
        m_skipPaths(skipPaths),
        m_geometryPaths(geometryPaths)
    {
    }

    bool load(std::istream &in, const std::filesystem::path &dir)
    {
        return m_objDB->load(in, dir);
    }

    void process(const std::filesystem::path &filePath, const std::filesystem::path &parentPath)
    {
        if (DetermineFileType(filePath) != m_contentType)
            return;

        if (std::find(std::begin(m_skipPaths), std::end(m_skipPaths), filePath)
            != std::end(m_skipPaths))
        {
            util::GetLogger()->info(_("Skipping {} catalog: {}\n"), m_typeDesc, filePath);
            return;
        }
        util::GetLogger()->info(_("Loading {} catalog: {}\n"), m_typeDesc, filePath);
        if (m_notifier != nullptr)
            m_notifier->update(filePath.filename().string());

        if (std::ifstream catalogFile(filePath);
            !catalogFile.good() || !load(catalogFile, parentPath))
        {
            util::GetLogger()->error(_("Error reading {} catalog file: {}\n"),
                                     m_typeDesc,
                                     filePath);
        }
    }

    void loadExtras(util::array_view<std::filesystem::path> dirs)
    {
        std::vector<std::filesystem::path> entries;
        std::error_code       ec;
        for (const auto &dir : dirs)
        {
            if (!util::IsValidDirectory(dir))
                continue;

            entries.clear();

            for (auto iter = std::filesystem::recursive_directory_iterator(dir, ec); iter != end(iter);
                 iter.increment(ec))
            {
                if (ec)
                    continue;
                if (!std::filesystem::is_directory(iter->path(), ec))
                    entries.push_back(iter->path());
            }

            std::sort(std::begin(entries), std::end(entries));

            for (const auto &fn : entries)
                process(fn, fn.parent_path());
        }
    }

private:
    OBJDB* m_objDB;
    std::string m_typeDesc;
    ContentType m_contentType;
    ProgressNotifier* m_notifier;
    util::array_view<std::filesystem::path> m_skipPaths;
    engine::GeometryPaths& m_geometryPaths;
};

} // namespace celestia
