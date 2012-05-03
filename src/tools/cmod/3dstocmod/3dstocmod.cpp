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

#include "convert3ds.h"
#include "cmodops.h"
#include <celmodel/modelfile.h>
#include <cel3ds/3dsread.h>
#include <cstring>
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace cmod;
using namespace std;


void usage()
{
    cerr << "Usage: 3dstocmod <input 3ds file>\n";
}


int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        usage();
        return 1;
    }

    string inputFileName = argv[1];

    cerr << "Reading...\n";
    M3DScene* scene = Read3DSFile(inputFileName);
    if (scene == NULL)
    {
        cerr << "Error reading 3DS file '" << inputFileName << "'\n";
        return 1;
    }

    cerr << "Converting...\n";
    Model* model = Convert3DSModel(*scene);
    if (!model)
    {
        cerr << "Error converting 3DS file to Celestia model\n";
        return 1;
    }

    // Generate normals for the model
    double smoothAngle = 45.0; // degrees
    double weldTolerance = 1.0e-6;
    bool weldVertices = true;

    Model* newModel = GenerateModelNormals(*model, float(smoothAngle * 3.14159265 / 180.0), weldVertices, weldTolerance);
    delete model;

    if (!newModel)
    {
        cerr << "Ran out of memory while generating surface normals.\n";
        return 1;
    }

    // Automatically uniquify vertices
    for (unsigned int i = 0; newModel->getMesh(i) != NULL; i++)
    {
        Mesh* mesh = newModel->getMesh(i);
        UniquifyVertices(*mesh);
    }

    model = newModel;

#if 0
    // Print information about primitive groups
    for (uint32 i = 0; model->getMesh(i); i++)
    {
        const Mesh* mesh = model->getMesh(i);
        for (uint32 j = 0; mesh->getGroup(j); j++)
        {
            const Mesh::PrimitiveGroup* group = mesh->getGroup(j);
        }
    }
#endif

    SaveModelAscii(model, cout);

    return 0;
}
