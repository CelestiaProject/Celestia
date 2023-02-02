// rotationmanager.cpp
//
// Copyright (C) 2006 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "rotationmanager.h"

#include <fstream>

#include <celephem/samporient.h>
#include <celutil/logger.h>

using celestia::util::GetLogger;


RotationModelManager* GetRotationModelManager()
{
    static RotationModelManager* rotationModelManager = nullptr;
    if (rotationModelManager == nullptr)
        rotationModelManager = std::make_unique<RotationModelManager>("data").release();
    return rotationModelManager;
}


fs::path RotationModelInfo::resolve(const fs::path& baseDir) const
{
    if (!path.empty())
    {
        fs::path filename = path / "data" / source;
        std::ifstream in(filename);
        if (in.good())
            return filename;
    }

    return baseDir / source;
}


std::unique_ptr<celestia::ephem::RotationModel>
RotationModelInfo::load(const fs::path& filename) const
{
    GetLogger()->verbose("Loading rotation model: {}\n", filename);

    return celestia::ephem::LoadSampledOrientation(filename);
}
