// rotationmanager.h
//
// Copyright (C) 2006 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef CELENGINE_ROTATIONMANAGER_H_
#define CELENGINE_ROTATIONMANAGER_H_

#include <string>
#include <map>
#include <celutil/resmanager.h>
#include <celengine/rotation.h>


class RotationModelInfo : public ResourceInfo<RotationModel>
{
 public:
    std::string source;
    std::string path;

    RotationModelInfo(const std::string _source,
                      const std::string _path = "") :
        source(_source), path(_path) {};

    virtual std::string resolve(const std::string&);
    virtual RotationModel* load(const std::string&);
};

inline bool operator<(const RotationModelInfo& ti0,
                      const RotationModelInfo& ti1)
{
    if (ti0.source == ti1.source)
        return ti0.path < ti1.path;
    else
        return ti0.source < ti1.source;
}

typedef ResourceManager<RotationModelInfo> RotationModelManager;

extern RotationModelManager* GetRotationModelManager();

#endif // CELENGINE_ROTATIONMANAGER_H_

