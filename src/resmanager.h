// resmanager.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _RESMANAGER_H_
#define _RESMANAGER_H_

#include <string>
#include <map>
#include "texture.h"


class ResourceManager
{
 public:
    ResourceManager();
    ResourceManager(std::string _baseDir);
    ResourceManager(char* _baseDir);
    ~ResourceManager();

 protected:
    bool findResource(const std::string& name, void**);
    void addResource(const std::string& name, void*);
    std::string baseDir;

 private:
    typedef std::map<std::string, void*> ResourceTable;

    ResourceTable resources;
};

#endif // _RESMANAGER_H_

