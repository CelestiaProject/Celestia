// directory.h
// 
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELUTIL_DIRECTORY_H_
#define _CELUTIL_DIRECTORY_H_

#include <string>
#include <vector>

class EnumFilesHandler
{
 public:
    EnumFilesHandler();
    virtual ~EnumFilesHandler() {};

    void pushDir(const std::string&);
    void popDir();
    const std::string& getPath() const; 

    virtual bool process(const std::string& filename) = 0;

 private:
    std::vector<std::string> dirStack;
};

class Directory
{
 public:
    Directory() {};
    virtual ~Directory() {};

    virtual bool nextFile(std::string&) = 0;

    virtual bool enumFiles(EnumFilesHandler& handler, bool deep);
};

extern std::string WordExp(const std::string&);
extern Directory* OpenDirectory(const std::string&);
extern bool IsDirectory(const std::string&);


#endif // _CELUTIL_DIRECTORY_H_
