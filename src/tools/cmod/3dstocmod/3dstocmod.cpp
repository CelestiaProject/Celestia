// 3dstocmod.cpp
//
// Copyright (C) 2004-2010, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// Convert a 3DS file to a Celestia mesh (.cmod) file

#include <iostream>
#include <memory>
#include <string>

#include <cel3ds/3dsread.h>
#include <celmath/mathlib.h>
#include <celutil/logger.h>

#include "cmodops.h"
#include "convert3ds.h"
#include "pathmanager.h"

using celestia::util::CreateLogger;

void usage()
{
    std::cerr << "Usage: 3dstocmod <input 3ds file>\n";
}


int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        usage();
        return 1;
    }

    CreateLogger();

    std::string inputFileName = argv[1];

    std::cerr << "Reading...\n";
    std::unique_ptr<M3DScene> scene = Read3DSFile(inputFileName);
    if (scene == nullptr)
    {
        std::cerr << "Error reading 3DS file '" << inputFileName << "'\n";
        return 1;
    }

    std::cerr << "Converting...\n";
    std::unique_ptr<cmod::Model> model = Convert3DSModel(*scene, GetPathManager()->getHandle);
    if (!model)
    {
        std::cerr << "Error converting 3DS file to Celestia model\n";
        return 1;
    }

    // Generate normals for the model
    float smoothAngle = 45.0; // degrees
    double weldTolerance = 1.0e-6;
    bool weldVertices = true;

    model = GenerateModelNormals(*model, celmath::degToRad(smoothAngle), weldVertices, weldTolerance);

    if (!model)
    {
        std::cerr << "Ran out of memory while generating surface normals.\n";
        return 1;
    }

    // Automatically uniquify vertices
    for (unsigned int i = 0; model->getMesh(i) != nullptr; i++)
    {
        cmod::Mesh* mesh = model->getMesh(i);
        UniquifyVertices(*mesh);
    }

    SaveModelAscii(model.get(), std::cout, GetPathManager()->getSource);

    return 0;
}
