// meshmanager.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_MESHMANAGER_H_
#define _CELENGINE_MESHMANAGER_H_

#include <string>
#include <map>
#include <celutil/resmanager.h>
#include <celengine/geometry.h>


class GeometryInfo : public ResourceInfo<Geometry>
{
 public:
    std::string source;
    std::string path;
    bool resolvedToPath;
    Eigen::Vector3f center;
    float scale;
    bool isNormalized;

    GeometryInfo(const std::string _source,
                 const std::string _path = "") :
        source(_source),
        path(_path),
        resolvedToPath(false),
        center(Eigen::Vector3f::Zero()),
        scale(1.0f),
        isNormalized(true)
        {};

    GeometryInfo(const std::string _source,
                 const std::string _path,
                 const Eigen::Vector3f& _center,
                 float _scale,
                 bool _isNormalized) :
        source(_source),
        path(_path),
        resolvedToPath(false),
        center(_center),
        scale(_scale),
        isNormalized(_isNormalized)
        {};

    virtual std::string resolve(const std::string&);
    virtual Geometry* load(const std::string&);
};

inline bool operator<(const GeometryInfo& g0, const GeometryInfo& g1)
{
    if (g0.source != g1.source)
        return g0.source < g1.source;
    else if (g0.path != g1.path)
        return g0.path < g1.path;
    else if (g0.isNormalized != g1.isNormalized)
        return (int) g0.isNormalized < (int) g1.isNormalized;
    else if (g0.scale != g1.scale)
        return g0.scale < g1.scale;
    else if (g0.center.x() != g1.center.x())
        return g0.center.x() < g1.center.x();
    else if (g0.center.y() != g1.center.y())
        return g0.center.y() < g1.center.y();
    else
        return g0.center.z() < g1.center.z();
}

typedef ResourceManager<GeometryInfo> GeometryManager;

extern GeometryManager* GetGeometryManager();

#endif // _CELENGINE_MESHMANAGER_H_

