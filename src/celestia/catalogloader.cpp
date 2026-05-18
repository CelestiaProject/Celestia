// catalogloader.cpp
//
// Copyright (C) 2001-2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "catalogloader.h"

#include <algorithm>
#include <fstream>
#include <system_error>

#include <celutil/fsutils.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include "progressnotifier.h"

namespace celestia
{

CatalogLoader::CatalogLoader(ProgressNotifier* notifier,
                             util::array_view<std::filesystem::path> skipPaths) :
    m_notifier(notifier),
    m_skipPaths(skipPaths)
{
}

void
CatalogLoader::process(const std::filesystem::path &filePath, const std::filesystem::path &parentPath)
{
    if (DetermineFileType(filePath) != contentType())
        return;

    if (std::find(m_skipPaths.begin(), m_skipPaths.end(), filePath) != m_skipPaths.end())
    {
        util::GetLogger()->info(_("Skipping {} catalog: {}\n"), typeDesc(), filePath);
        return;
    }

    util::GetLogger()->info(_("Loading {} catalog: {}\n"), typeDesc(), filePath);
    if (m_notifier)
        m_notifier->update(filePath.filename().string());

    if (std::ifstream catalogFile(filePath);
        !catalogFile.good() || !load(catalogFile, parentPath))
    {
        util::GetLogger()->error(_("Error reading {} catalog file: {}\n"),
                                    typeDesc(),
                                    filePath);
    }
}

void
CatalogLoader::loadExtras(util::array_view<std::filesystem::path> dirs)
{
    std::vector<std::filesystem::path> entries;
    std::error_code ec;
    for (const auto &dir : dirs)
    {
        if (!util::IsValidDirectory(dir))
            continue;

        entries.clear();

        for (auto iter = std::filesystem::recursive_directory_iterator(dir, ec); iter != std::filesystem::end(iter);
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

} // end namespace celestia
