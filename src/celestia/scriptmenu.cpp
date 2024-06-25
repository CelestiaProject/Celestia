// scriptmenu.cpp
//
// Copyright (C) 2007-2009, the Celestia Development Team
//
// Scan a directory and build a list of Celestia script files.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "scriptmenu.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <string_view>

#include <celutil/filetype.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>

using namespace std::string_view_literals;
using celestia::util::GetLogger;

namespace
{

constexpr std::string_view TitleTag = "Title:"sv;



void process(const fs::path& p, std::vector<ScriptMenuItem>& menuItems)
{
    auto type = DetermineFileType(p);
#ifndef CELX
    if (type != ContentType::CelestiaLegacyScript)
#else
    if (type != ContentType::CelestiaScript &&
        type != ContentType::CelestiaLegacyScript)
#endif
        return;

    // Scan the script file for metainformation. At the moment,
    // the only thing searched for is the script title, which must
    // appear on the first line after the string 'Title:'
    std::ifstream in(p);
    if (!in.good())
        return;

    ScriptMenuItem& item = menuItems.emplace_back();
    item.title = p.filename().string();
    item.filename = p;

    // Read the first line, handling various newline conventions
    std::array<char, 512> buffer;
    in.getline(buffer.data(), buffer.size());
    // Delimiter is extracted and contributes to gcount() but is not stored
    std::size_t lineLength;
    if (in.good())
        lineLength = static_cast<std::size_t>(in.gcount() - 1);
    else if (in.eof())
        lineLength = static_cast<std::size_t>(in.gcount());
    else
        return;

    std::string_view line(buffer.data(), lineLength);

    // Skip whitespace before 'Title:' tag
    if (auto pos = line.find_first_not_of(" \t"sv); pos == std::string_view::npos)
        return;
    else
        line = line.substr(pos);

    // Check for 'Title:' tag
    if (line.size() < TitleTag.size() || line.substr(0, TitleTag.size()) != TitleTag)
        return;
    else
        line = line.substr(TitleTag.size());

    // Skip whitespace after 'Title: tag
    if (auto pos = line.find_first_not_of(" \t"sv); pos == std::string_view::npos)
        return;
    else
        line = line.substr(pos);

    // Trim trailing whitespace
    if (auto pos = line.find_last_not_of(" \t"sv); pos == std::string_view::npos)
        return;
    else
        item.title = line.substr(0, pos + 1);
}

} // end unnamed namespace

std::vector<ScriptMenuItem>
ScanScriptsDirectory(const fs::path& scriptsDir, bool deep)
{
    std::vector<ScriptMenuItem> scripts;

    if (scriptsDir.empty())
        return scripts;

    std::error_code ec;
    if (!fs::is_directory(scriptsDir, ec))
    {
        GetLogger()->warn(_("Path {} doesn't exist or isn't a directory\n"), scriptsDir);
        return scripts;
    }

    if (deep)
    {
        for (auto iter = fs::recursive_directory_iterator(scriptsDir, ec); iter != end(iter); iter.increment(ec))
        {
            if (ec)
                continue;
            process(*iter, scripts);
        }
    }
    else
    {
        for (const auto& p : fs::directory_iterator(scriptsDir, ec))
        {
            if (ec)
                continue;
            process(p, scripts);
        }
    }

    return scripts;
}
