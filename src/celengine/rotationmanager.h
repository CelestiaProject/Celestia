// rotationmanager.h
//
// Copyright (C) 2006 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>
#include <string>
#include <tuple>

#include <celcompat/filesystem.h>
#include <celephem/rotation.h>
#include <celutil/resmanager.h>


class RotationModelInfo
{
 private:
    std::string source;
    fs::path path;

    friend bool operator<(const RotationModelInfo&, const RotationModelInfo&);

 public:
    using ResourceType = celestia::ephem::RotationModel;
    using ResourceKey = fs::path;

    RotationModelInfo(const std::string& _source,
                      const fs::path& _path = "") :
        source(_source), path(_path) {};

    fs::path resolve(const fs::path&) const;
    std::unique_ptr<celestia::ephem::RotationModel> load(const fs::path&) const;
};

inline bool operator<(const RotationModelInfo& ti0,
                      const RotationModelInfo& ti1)
{
    return std::tie(ti0.source, ti0.path) < std::tie(ti1.source, ti1.path);
}

typedef ResourceManager<RotationModelInfo> RotationModelManager;

extern RotationModelManager* GetRotationModelManager();
