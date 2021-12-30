// spheremesh.h
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <Eigen/Core>

#include <celengine/dispmap.h>

namespace cmod
{
class Mesh;
}

/*! The SphereMesh class is used to generate displacement mapped
 *  spheres when loading the now-deprecated .cms geometry files.
 */
class SphereMesh
{
public:
    SphereMesh(const Eigen::Vector3f& size,
               int _nRings, int _nSlices,
               DisplacementMapFunc func,
               void* info);
    ~SphereMesh();

    //! Convert this object into a standard Celestia mesh.
    cmod::Mesh convertToMesh() const;

 private:
    void createSphere(float radius, int _nRings, int _nSlices);
    void generateNormals();
    void scale(const Eigen::Vector3f&);
    void fixNormals();
    void displace(const DisplacementMap& dispmap, float height);
    void displace(DisplacementMapFunc func, void* info);

    int nRings;
    int nSlices;
    int nVertices;
    float* vertices{ nullptr };
    float* normals{ nullptr };
    float* texCoords{ nullptr };
    float* tangents{ nullptr };
    int nIndices;
    unsigned short* indices{ nullptr };
};
