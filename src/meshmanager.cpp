// meshmanager.cpp
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
#include "mathlib.h"
#include "parser.h"

#include "3dsread.h"
#include "3dsmesh.h"
#include "spheremesh.h"
#include "perlin.h"
#include "filetype.h"
#include "meshmanager.h"

using namespace std;


static Mesh* LoadCelestiaMesh(const string& filename);

static MeshManager* meshManager = NULL;


MeshManager* GetMeshManager()
{
    if (meshManager == NULL)
        meshManager = new MeshManager("models");
    return meshManager;
}


Mesh* MeshInfo::load(const string& baseDir)
{
    DPRINTF("Loading mesh: %s\n", source.c_str());
    ContentType fileType = DetermineFileType(source);

    if (fileType == Content_3DStudio)
    {
        Mesh3DS* mesh3 = NULL;
        M3DScene* scene = Read3DSFile(baseDir + '/' + source);
        if (scene != NULL)
        {
            mesh3 = new Mesh3DS(*scene);
            mesh3->normalize();
            delete scene;
            return mesh3;
        }
    }
    else if (fileType == Content_CelestiaMesh)
    {
        return LoadCelestiaMesh(baseDir + '/' + source);
    }
 
    return NULL;
}


struct NoiseMeshParameters
{
    Vec3f size;
    Vec3f offset;
    float featureHeight;
    float octaves;
    float slices;
    float rings;
};

static float NoiseDisplacementFunc(float u, float v, void* info)
{
    float theta = u * (float) PI * 2;
    float phi = (v - 0.5f) * (float) PI;
    float x = (float) (cos(phi) * cos(theta));
    float y = (float) sin(phi);
    float z = (float) (cos(phi) * sin(theta));

    // assert(info != NULL);
    NoiseMeshParameters* params = (NoiseMeshParameters*) info;

    return fractalsum(Point3f(x, y, z) + params->offset,
                      (int) params->octaves) * params->featureHeight;
}

Mesh* LoadCelestiaMesh(const string& filename)
{
    ifstream meshFile(filename.c_str(), ios::in);
    if (!meshFile.good())
    {
        DPRINTF("Error opening mesh file: %s\n", filename.c_str());
        return NULL;
    }

    Tokenizer tokenizer(&meshFile);
    Parser parser(&tokenizer);

    if (tokenizer.nextToken() != Tokenizer::TokenName)
    {
        DPRINTF("Mesh file %s is invalid.\n", filename.c_str());
        return NULL;
    }

    if (tokenizer.getStringValue() != "SphereDisplacementMesh")
    {
        DPRINTF("%s: Unrecognized mesh type %s.\n",
                filename.c_str(),
                tokenizer.getStringValue().c_str());
        return NULL;
    }

    Value* meshDefValue = parser.readValue();
    if (meshDefValue == NULL)
    {
        DPRINTF("%s: Bad mesh file.\n", filename.c_str());
        return NULL;
    }

    if (meshDefValue->getType() != Value::HashType)
    {
        DPRINTF("%s: Bad mesh file.\n", filename.c_str());
        delete meshDefValue;
        return NULL;
    }

    Hash* meshDef = meshDefValue->getHash();
    
    NoiseMeshParameters params;
    
    params.size = Vec3f(1, 1, 1);
    params.offset = Vec3f(10, 10, 10);
    params.featureHeight = 0.0f;
    params.octaves = 1;
    params.slices = 20;
    params.rings = 20;

    meshDef->getVector("Size", params.size);
    meshDef->getVector("NoiseOffset", params.offset);
    meshDef->getNumber("FeatureHeight", params.featureHeight);
    meshDef->getNumber("Octaves", params.octaves);
    meshDef->getNumber("Slices", params.slices);
    meshDef->getNumber("Rings", params.rings);

    delete meshDefValue;

    cout << "Read Celestia mesh " << filename << " successfully!";

    return new SphereMesh(params.size,
                          (int) params.rings, (int) params.slices,
                          NoiseDisplacementFunc,
                          (void*) &params);
}
