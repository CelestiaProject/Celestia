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
#include "celutil/directory.h"
#include "celutil/filetype.h"
#include <fstream>

using namespace std;


static const string TitleTag("Title:");

class ScriptScanner : public EnumFilesHandler
{
public:
    ScriptScanner() :
        menuItems(NULL)
    {
    }

    bool process(const string& filename)
    {
        if (
#ifdef CELX
            DetermineFileType(filename) == Content_CelestiaScript ||
#endif
            DetermineFileType(filename) == Content_CelestiaLegacyScript
            )
        {
            string filepath = getPath() + string("/") + filename;

            // Scan the script file for metainformation. At the moment,
            // the only thing searched for is the script title, which must
            // appear on the first line after the string 'Title:'
            ifstream in(filepath.c_str());
            if (in.good())
            {
                ScriptMenuItem item;
                item.filename = filepath;

                // Read the first line, handling various newline conventions
                char firstLineBuf[512];
                unsigned int count = 0;
                while (count < sizeof(firstLineBuf) - 1 && in.good())
                {
                    int c = in.get();
                    if (c == '\n' || c == '\r')
                        break;
                    firstLineBuf[count++] = c;
                }

                string firstLine(firstLineBuf, count);
                string::size_type titlePos = firstLine.find(TitleTag);

                // Skip spaces after the Title: tag
                if (titlePos != string::npos)
                    titlePos = firstLine.find_first_not_of(" ", titlePos + TitleTag.length());

                if (titlePos != string::npos)
                {
                    item.title = firstLine.substr(titlePos);
                }
                else
                {
                    // No title tag--just use the filename
                    item.title = filename;
                }

                menuItems->push_back(item);
            }
        }

        return true;
    }

    vector<ScriptMenuItem>* menuItems;
};



std::vector<ScriptMenuItem>*
ScanScriptsDirectory(string scriptsDir, bool deep)
{
    vector<ScriptMenuItem>* scripts = new vector<ScriptMenuItem>;
    if (scripts == NULL)
        return NULL;

    Directory* dir = OpenDirectory(scriptsDir);

    ScriptScanner scanner;
    scanner.menuItems = scripts;
    scanner.pushDir(scriptsDir);

    dir->enumFiles(scanner, deep);
    delete dir;

    return scripts;
}

