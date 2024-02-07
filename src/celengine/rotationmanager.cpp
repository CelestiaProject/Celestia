// rotationmanager.cpp
//
// Copyright (C) 2006 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "rotationmanager.h"

#include <celephem/samporient.h>
#include <celutil/logger.h>

using celestia::util::GetLogger;

namespace celestia::engine
{

std::shared_ptr<const ephem::RotationModel>
RotationModelManager::find(const fs::path& source,
                           const fs::path& path)
{
    auto filename = path.empty()
        ? "data" / source
        : path / "data" / source;

    auto it = rotationModels.try_emplace(filename).first;
    if (auto cachedModel = it->second.lock(); cachedModel != nullptr)
        return cachedModel;

    GetLogger()->verbose("Loading rotation model: {}\n", filename);
    auto model = ephem::LoadSampledOrientation(filename);
    if (model == nullptr)
    {
        rotationModels.erase(it);
        return nullptr;
    }

    it->second = model;
    return model;
}

RotationModelManager*
GetRotationModelManager()
{
    static RotationModelManager* const manager = std::make_unique<RotationModelManager>().release(); //NOSONAR
    return manager;
}

} // end namespace celestia::engine
