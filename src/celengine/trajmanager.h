// trajmanager.h
//
// Copyright (C) 2001-2007 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef CELENGINE_TRAJMANAGER_H_
#define CELENGINE_TRAJMANAGER_H_

#include <string>
#include <map>
#include <celutil/resmanager.h>
#include <celengine/orbit.h>
#include <celengine/samporbit.h>


class TrajectoryInfo : public ResourceInfo<Orbit>
{
 public:
    std::string source;
    std::string path;
    TrajectoryInterpolation interpolation;
    TrajectoryPrecision precision;

    TrajectoryInfo(const std::string _source,
                   const std::string _path = "",
                   TrajectoryInterpolation _interpolation = TrajectoryInterpolationCubic,
                   TrajectoryPrecision _precision = TrajectoryPrecisionSingle) :
        source(_source), path(_path), interpolation(_interpolation), precision(_precision) {};

    virtual std::string resolve(const std::string&);
    virtual Orbit* load(const std::string&);
};

// Sort trajectory info records. The same trajectory can be loaded multiple times with
// different attributes for precision and interpolation. How the ordering is defined isn't
// as important making this operator distinguish between trajectories with either different
// sources or different attributes.
inline bool operator<(const TrajectoryInfo& ti0, const TrajectoryInfo& ti1)
{
    if (ti0.interpolation == ti1.interpolation)
    {
        if (ti0.precision == ti1.precision)
        {
            if (ti0.source == ti1.source)
                return ti0.path < ti1.path;
            else
                return ti0.source < ti1.source;
        }
        else
        {
            return ti0.precision < ti1.precision;
        }
    }
    else
    {
        return ti0.interpolation < ti1.interpolation;
    }
}

typedef ResourceManager<TrajectoryInfo> TrajectoryManager;

extern TrajectoryManager* GetTrajectoryManager();

#endif // CELENGINE_TRAJMANAGER_H_

