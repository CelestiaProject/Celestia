// directory.h
// 
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include "directory.h"

using namespace std;


bool Directory::enumFiles(EnumFilesHandler& handler, bool deep)
{
    string filename;

    while (nextFile(filename))
    {
        // Skip all files beginning with a period, most importantly, . and ..
        if (filename[0] == '.')
            continue;

        // TODO: optimize this to avoid allocating so many strings
        string pathname = handler.getPath() + string("/") + filename;
        if (IsDirectory(pathname))
        {
            if (deep)
            {
                Directory* dir = OpenDirectory(pathname);
                bool cont = true;

                if (dir != NULL)
                {
                    handler.pushDir(filename);
                    cont = dir->enumFiles(handler, deep);
                    handler.popDir();
                    delete dir;
                }

                if (!cont)
                    return false;
            }
        }
        else
        {
            if (!handler.process(filename))
                return false;
        }
    }

    return true;
}


EnumFilesHandler::EnumFilesHandler()
{
}


void EnumFilesHandler::pushDir(const std::string& dirName)
{
    if (dirStack.size() > 0)
        dirStack.push_back(dirStack.back() + string("/") + dirName);
    else
        dirStack.push_back(dirName);
}


void EnumFilesHandler::popDir()
{
    dirStack.pop_back();
}


const string& EnumFilesHandler::getPath() const
{
    // need this so we can return a non-temporary value:
    static const string emptyString("");

    if (dirStack.size() > 0)
        return dirStack.back();
    else
        return emptyString;
}
