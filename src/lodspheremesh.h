// lodspheremesh.h
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _LODSPHEREMESH_H_
#define _LODSPHEREMESH_H_

#include "vecmath.h"
#include "mesh.h"
#include "dispmap.h"

class LODSphereMesh : public Mesh
{
public:
    LODSphereMesh();
    ~LODSphereMesh();

    void render();
    void render(unsigned int attributes);

 private:
    void SphereMesh::renderSection(int phi0, int theta0,
                                   int phi1, int theta1,
                                   int phiStep, int thetaStep,
                                   unsigned int attributes);

    int nRings;
    int nSlices;
    int nVertices;
    float* vertices;
    float* normals;
    float* texCoords;
    float* tangents;
    int nIndices;
    unsigned short* indices;
};

#endif // _SPHEREMESH_H_
