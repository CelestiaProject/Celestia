// mesh.h
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _SPHEREMESH_H_
#define _SPHEREMESH_H_

#include "vecmath.h"
#include "mesh.h"
#include "dispmap.h"

class SphereMesh : public Mesh
{
public:
    SphereMesh(float radius, int _nRings, int _nSlices);
    SphereMesh(Vec3f size, int _nRings, int _nSlices);
    SphereMesh(Vec3f size,
               const DisplacementMap& dispmap,
               float height = 1.0f);
    SphereMesh(Vec3f size,
               int _nRings, int _nSlices,
               DisplacementMapFunc func,
               void* info);
    ~SphereMesh();

    void render(float lod);
    void render(unsigned int attributes, float lod);

 private:
    void createSphere(float radius, int nRings, int nSlices);
    void generateNormals();
    void scale(Vec3f);
    void fixNormals();
    void displace(const DisplacementMap& dispmap, float height);
    void displace(DisplacementMapFunc func, void* info);

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
