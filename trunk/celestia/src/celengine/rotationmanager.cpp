// rotationmanager.cpp
//
// Copyright (C) 2006 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "rotationmanager.h"
#include "celestia.h"
#include <celephem/samporient.h>
#include <celutil/debug.h>
#include <iostream>
#include <fstream>

using namespace std;


static RotationModelManager* rotationModelManager = NULL;


RotationModelManager* GetRotationModelManager()
{
    if (rotationModelManager == NULL)
        rotationModelManager = new RotationModelManager("data");
    return rotationModelManager;
}


string RotationModelInfo::resolve(const string& baseDir)
{
    if (!path.empty())
    {
        string filename = path + "/data/" + source;
        ifstream in(filename.c_str());
        if (in.good())
            return filename;
    }

    return baseDir + "/" + source;
}


RotationModel* RotationModelInfo::load(const string& filename)
{
    DPRINTF(1, "Loading rotation model: %s\n", filename.c_str());

    return LoadSampledOrientation(filename);
}
