// resmanager.cpp
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "resmanager.h"

using namespace std;


ResourceManager::ResourceManager() : baseDir("")
{
}


ResourceManager::ResourceManager(string _baseDir) : baseDir(_baseDir)
{
}


ResourceManager::ResourceManager(char* _baseDir) : baseDir(_baseDir)
{
}


ResourceManager::~ResourceManager()
{
    // TODO: Clean up
}


// If the named resource has already been loaded, return a pointer to it
// in res and return true.  Otherwise, leave res untouched and return false.
// find() can be safely called with res == NULL just to test whether or not
// the resource is resident.
bool ResourceManager::findResource(string name, void** res)
{
    ResourceTable::iterator iter = resources.find(name);
    if (iter != resources.end())
    {
        if (res != NULL)
            *res = iter->second;
        return true;
    }
    else
    {
        return false;
    }
}


void ResourceManager::addResource(string name, void* res)
{
    resources.insert(ResourceTable::value_type(name, res));
}

