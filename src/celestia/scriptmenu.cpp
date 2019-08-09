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
#include "celutil/filesystem.h"
#include "celutil/filetype.h"
#include <fstream>

using namespace std;


static const char TitleTag[] = "Title:";

static void process(const fs::path& p, vector<ScriptMenuItem>* menuItems)
{
#ifndef CELX
    if (DetermineFileType(p) != Content_CelestiaLegacyScript)
#else
    if (DetermineFileType(p) != Content_CelestiaScript &&
        DetermineFileType(p) != Content_CelestiaLegacyScript)
#endif
        return;

    // Scan the script file for metainformation. At the moment,
    // the only thing searched for is the script title, which must
    // appear on the first line after the string 'Title:'
    ifstream in(p.string());
    if (in.good())
    {
        ScriptMenuItem item;
        item.filename = p;

        // Read the first line, handling various newline conventions
        char firstLineBuf[512];
        size_t count = 0;
        while (count < sizeof(firstLineBuf) - 1 && in.good())
        {
            int c = in.get();
            if (c == '\n' || c == '\r')
                break;
            firstLineBuf[count++] = c;
        }

        string firstLine(firstLineBuf, count);
        auto titlePos = firstLine.find(TitleTag);

        // Skip spaces after the Title: tag
        if (titlePos != string::npos)
            titlePos = firstLine.find_first_not_of(' ', titlePos + (sizeof(TitleTag) - 1));

        if (titlePos != string::npos)
        {
            item.title = firstLine.substr(titlePos);
        }
        else
        {
            // No title tag--just use the filename
            item.title = p.filename().string();
        }
        menuItems->push_back(item);
    }
}

vector<ScriptMenuItem>*
ScanScriptsDirectory(const fs::path& scriptsDir, bool deep)
{
    vector<ScriptMenuItem>* scripts = new vector<ScriptMenuItem>;

    for (const auto& p : fs::recursive_directory_iterator(scriptsDir))
        process(p, scripts);

    return scripts;
}
