// trajmanager.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
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


class TrajectoryInfo : public ResourceInfo<Orbit>
{
 public:
    std::string source;
    std::string path;

    TrajectoryInfo(const std::string _source,
                   const std::string _path = "") :
        source(_source), path(_path) {};

    virtual std::string resolve(const std::string&);
    virtual Orbit* load(const std::string&);
};

inline bool operator<(const TrajectoryInfo& ti0, const TrajectoryInfo& ti1)
{
    if (ti0.source == ti1.source)
        return ti0.path < ti1.path;
    else
        return ti0.source < ti1.source;
}

typedef ResourceManager<TrajectoryInfo> TrajectoryManager;

extern TrajectoryManager* GetTrajectoryManager();

#endif // CELENGINE_TRAJMANAGER_H_

