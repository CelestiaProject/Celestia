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

#if 0
class MeshManager : public ResourceManager
{
 public:
    MeshManager() : ResourceManager() {};
    MeshManager(std::string _baseDir) : ResourceManager(_baseDir) {};
    MeshManager(char* _baseDir) : ResourceManager(_baseDir) {};
    ~MeshManager();

    bool find(const std::string& name, Mesh**);
    Mesh* load(const std::string& name);
};
#endif

class MeshInfo : public ResourceInfo<Mesh>
{
 public:
    std::string source;

    MeshInfo(const std::string _source) : source(_source) {};

    virtual Mesh* load(const std::string&);
};

inline bool operator<(const MeshInfo& mi0, const MeshInfo& mi1)
{
    return mi0.source < mi1.source;
}

typedef ResourceManager<MeshInfo> MeshManager;

extern MeshManager* GetMeshManager();

#endif // _MESHMANAGER_H_

