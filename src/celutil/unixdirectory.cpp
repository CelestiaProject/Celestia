// unixdirectory.cpp
// 
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <dirent.h>
#include "directory.h"

using namespace std;


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
