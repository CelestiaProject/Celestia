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
#include <shlobj.h>
#ifdef _MSC_VER
#include <direct.h>
#else
#include <sys/stat.h>
#endif
#include <cstdlib>
#include "directory.h"
#include "util.h"

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


static string ToWinSeparators(const string& filename)
{
    string::size_type pos = filename.find('/');
    if (pos == string::npos)
        return filename;

    return filename.substr(0, pos) + '\\' + ToWinSeparators(filename.substr(pos + 1));
}


string WordExp(const string& filename)
{
    if (filename[0] == '~')
    {
        if (filename.size() == 1)
            return HomeDir();
        if (filename[1] == '\\' || filename[1] == '/')
            return HomeDir() + '\\' + ToWinSeparators(filename.substr(2));
    }
    return filename;
}


string HomeDir()
{
    char p[MAX_PATH + 1];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, &p[0])))
        return p;

    // fallback to environment variables
    const char *s = getenv("USERPROFILE");
    if (s != NULL)
        return s;

    const char *s1 = getenv("HOMEDRIVE");
    const char *s2 = getenv("HOMEPATH");
    if (s1 != NULL && s2 != NULL)
    {
        return string(s1) + '/' + s2;
    }

    // unlikely this is defined in woe but let's check
    s = getenv("HOME");
    if (s != NULL)
        return s;

    return "";
}


bool IsAbsolutePath(const std::string &p)
{
    return    p[0] == '/'
           || p[0] == '\\'
           || (((p[0] >= 'A' && p[0] <= 'Z') || (p[0] >= 'a' && p[0] <= 'z')) && p[1] == ':');
}

string WriteableDataPath()
{
    const char *p = getenv("APPDATA");
    p = p != NULL ? p : "~\\AppData\\Roaming";
    return WordExp(p) + "\\Celestia";
}

bool MkDir(const std::string &p)
{
#ifdef _MSC_VER
    return _mkdir(p.c_str()) == 0;
#else
    return mkdir(p.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
#endif
}
