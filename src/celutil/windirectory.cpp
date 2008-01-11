// windirectory.cpp
// 
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include <windows.h>
#include "directory.h"

using namespace std;


class WindowsDirectory : public Directory
{
public:
    WindowsDirectory(const std::string&);
    virtual ~WindowsDirectory();

    virtual bool nextFile(std::string&);

    enum {
        DirGood = 0,
        DirBad = 1
    };

private:
    string dirname;
    string searchName;
    int status;
    HANDLE searchHandle;
};


WindowsDirectory::WindowsDirectory(const std::string& _dirname) :
    dirname(_dirname),
    status(DirGood),
    searchHandle(INVALID_HANDLE_VALUE)
{
    searchName = dirname + string("\\*");
    // Check to make sure that this file is a directory
}


WindowsDirectory::~WindowsDirectory()
{
    if (searchHandle != INVALID_HANDLE_VALUE)
        FindClose(searchHandle);
    searchHandle = NULL;
}


bool WindowsDirectory::nextFile(std::string& filename)
{
    WIN32_FIND_DATAA findData;

    if (status != DirGood)
        return false;

    if (searchHandle == INVALID_HANDLE_VALUE)
    {
        searchHandle = FindFirstFileA(const_cast<char*>(searchName.c_str()),
                                      &findData);
        if (searchHandle == INVALID_HANDLE_VALUE)
        {
            status = DirBad;
            return false;
        }
        else
        {
            filename = findData.cFileName;
            return true;
        }
    }
    else
    {
        if (FindNextFileA(searchHandle, &findData))
        {
            filename = findData.cFileName;
            return true;
        }
        else
        {
            status = DirBad;
            return false;
        }
    }
}


Directory* OpenDirectory(const std::string& dirname)
{
    return new WindowsDirectory(dirname);
}


bool IsDirectory(const std::string& filename)
{
    DWORD attr = GetFileAttributesA(const_cast<char*>(filename.c_str()));
    if (attr == 0xffffffff)
        return false;
    else
        return ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0);
}

std::string WordExp(const std::string& filename) {
    return filename;
}
