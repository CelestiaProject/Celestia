// meshmanager.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_MODELMANAGER_H_
#define _CELENGINE_MODELMANAGER_H_

#include <string>
#include <map>
#include <celutil/resmanager.h>
#include <celengine/model.h>


class ModelInfo : public ResourceInfo<Model>
{
 public:
    std::string source;
    std::string path;
    bool resolvedToPath;
    Vec3f center;

    ModelInfo(const std::string _source,
              const std::string _path = "") :
        source(_source),
        path(_path),
        resolvedToPath(false),
        center(0.0, 0.0f, 0.0f)
        {};

    ModelInfo(const std::string _source,
              const std::string _path,
              const Vec3f& _center) :
        source(_source),
        path(_path),
        resolvedToPath(false),
        center(_center)
        {};

    virtual std::string resolve(const std::string&);
    virtual Model* load(const std::string&);
};

inline bool operator<(const ModelInfo& mi0, const ModelInfo& mi1)
{
    if (mi0.source != mi1.source)
        return mi0.source < mi1.source;
    else if (mi0.path != mi1.path)
        return mi0.path < mi1.path;
    else if (mi0.center.x != mi1.center.x)
        return mi0.center.x < mi1.center.x;
    else if (mi0.center.y != mi1.center.y)
        return mi0.center.y < mi1.center.y;
    else
        return mi0.center.z < mi1.center.z;
}

typedef ResourceManager<ModelInfo> ModelManager;

extern ModelManager* GetModelManager();

#endif // _CELENGINE_MODELMANAGER_H_

