// unixdirectory.cpp
// 
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <wordexp.h>
#include "directory.h"

using namespace std;


#ifdef TARGET_OS_MAC
#ifdef QT_CORE_LIB
// Crash on Mac OS X / Qt4 version when calling wordfree.
// This seems to happen only with Leopard.
#define WORDEXP_PROBLEM
#endif
#endif

class UnixDirectory : public Directory
{
public:
    UnixDirectory(const std::string&);
    virtual ~UnixDirectory();

    virtual bool nextFile(std::string&);

    enum {
        DirGood = 0,
        DirBad = 1
    };

private:
    string dirname;
    int status;
    DIR* dir;
};


UnixDirectory::UnixDirectory(const std::string& _dirname) :
    dirname(_dirname),
    status(DirGood),
    dir(NULL)
{
}


UnixDirectory::~UnixDirectory()
{
    if (dir != NULL)
    {
        closedir(dir);
        dir = NULL;
    }
}


bool UnixDirectory::nextFile(std::string& filename)
{
    if (status != DirGood)
        return false;

    if (dir == NULL)
    {
        dir = opendir(dirname.c_str());
        if (dir == NULL)
        {
            status = DirBad;
            return false;
        }
    }

    struct dirent* ent = readdir(dir);
    if (ent == NULL)
    {
        status = DirBad;
        return false;
    }
    else
    {
        filename = ent->d_name;
        return true;
    }
}


Directory* OpenDirectory(const std::string& dirname)
{
    return new UnixDirectory(dirname);
}


bool IsDirectory(const std::string& filename)
{
    struct stat buf;
    stat(filename.c_str(), &buf);
    return S_ISDIR(buf.st_mode);
}

std::string WordExp(const std::string& filename) 
{
#ifndef WORDEXP_PROBLEM   
    wordexp_t result;
    std::string expanded;

    switch(wordexp(filename.c_str(), &result, WRDE_NOCMD)) {
    case 0: // successful
        break;
    case WRDE_NOSPACE:
            // If the error was `WRDE_NOSPACE',
            // then perhaps part of the result was allocated.
        wordfree(&result);
    default: // some other error
        return filename;
    }

    if (result.we_wordc != 1) {
        wordfree(&result);
        return filename;
    }

    expanded = result.we_wordv[0];
    wordfree(&result);
#else
    std::string expanded = filename;
#endif
    return expanded;
}
