// trajmanager.cpp
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include <fstream>

#include "celestia.h"
#include <celutil/debug.h>
#include "samporbit.h"
#include "trajmanager.h"

using namespace std;


static TrajectoryManager* trajectoryManager = NULL;


TrajectoryManager* GetTrajectoryManager()
{
    if (trajectoryManager == NULL)
        trajectoryManager = new TrajectoryManager("data");
    return trajectoryManager;
}


string TrajectoryInfo::resolve(const string& baseDir)
{
    if (!path.empty())
    {
        string filename = path + "/data/" + source;
        // cout << "Resolve: testing [" << filename << "]\n";
        ifstream in(filename.c_str());
        if (in.good())
            return filename;
    }

    return baseDir + "/" + source;
}

Orbit* TrajectoryInfo::load(const string& filename)
{
    DPRINTF(1, "Loading trajectory: %s\n", filename.c_str());
    cout << "Loading trajectory: " << filename << '\n';

    return LoadSampledOrbit(filename);
}
