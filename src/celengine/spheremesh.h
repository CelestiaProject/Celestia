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

#include <celmath/vecmath.h>
#include <celmath/frustum.h>
#include <celengine/mesh.h>
#include <celengine/dispmap.h>

/*! The SphereMesh class is used to generate displacement mapped
 *  spheres when loading the now-deprecated .cms geometry files.
 *  It remains in the Celestia code base for backward compatibility,
 *  and it's use is discouraged.
 */
class SphereMesh
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

    //! Convert this object into a standard Celestia mesh.
    Mesh* convertToMesh() const;

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
