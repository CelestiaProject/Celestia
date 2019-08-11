// rotationmanager.cpp
//
// Copyright (C) 2006 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "rotationmanager.h"
#include <config.h>
#include <celephem/samporient.h>
#include <celutil/debug.h>
#include <iostream>
#include <fstream>

using namespace std;


static RotationModelManager* rotationModelManager = nullptr;


RotationModelManager* GetRotationModelManager()
{
    if (rotationModelManager == nullptr)
        rotationModelManager = new RotationModelManager("data");
    return rotationModelManager;
}


fs::path RotationModelInfo::resolve(const fs::path& baseDir)
{
    if (!path.empty())
    {
        fs::path filename = path / "data" / source;
        ifstream in(filename);
        if (in.good())
            return filename;
    }

    return baseDir / source;
}


RotationModel* RotationModelInfo::load(const fs::path& filename)
{
    DPRINTF(1, "Loading rotation model: %s\n", filename.c_str());

    return LoadSampledOrientation(filename);
}
