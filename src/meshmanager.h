// meshmanager.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _MESHMANAGER_H_
#define _MESHMANAGER_H_

#include <string>
#include <map>
#include "mesh.h"
#include "resmanager.h"


class MeshManager : public ResourceManager
{
 public:
    MeshManager() : ResourceManager() {};
    MeshManager(string _baseDir) : ResourceManager(_baseDir) {};
    MeshManager(char* _baseDir) : ResourceManager(_baseDir) {};
    ~MeshManager();

    bool find(string name, Mesh**);
    Mesh* load(string name);
};

#endif // _MESHMANAGER_H_

