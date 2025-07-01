// rotationmanager.h
//
// Copyright (C) 2006 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <filesystem>
#include <memory>
#include <tuple>
#include <unordered_map>

#include <celephem/rotation.h>
#include <celutil/fsutils.h>

namespace celestia::engine
{

class RotationModelManager
{
public:
    RotationModelManager() = default;
    ~RotationModelManager() = default;

    RotationModelManager(const RotationModelManager&) = delete;
    RotationModelManager& operator=(const RotationModelManager&) = delete;

    std::shared_ptr<const ephem::RotationModel> find(const std::filesystem::path& source,
                                                     const std::filesystem::path& path);

private:
    std::unordered_map<std::filesystem::path, std::weak_ptr<const ephem::RotationModel>, util::PathHasher> rotationModels;
};

RotationModelManager* GetRotationModelManager();

} // end namespace celestia::engine
