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
#include <celutil/resmanager.h>
#include <celengine/mesh.h>


class MeshInfo : public ResourceInfo<Mesh>
{
 public:
    std::string source;
    std::string path;
    bool resolvedToPath;

    MeshInfo(const std::string _source,
             const std::string _path = "") :
        source(_source), path(_path), resolvedToPath(false) {};

    virtual std::string resolve(const std::string&);
    virtual Mesh* load(const std::string&);
};

inline bool operator<(const MeshInfo& mi0, const MeshInfo& mi1)
{
    if (mi0.source == mi1.source)
        return mi0.path < mi1.path;
    else
        return mi0.source < mi1.source;
}

typedef ResourceManager<MeshInfo> MeshManager;

extern MeshManager* GetMeshManager();

#endif // _MESHMANAGER_H_

