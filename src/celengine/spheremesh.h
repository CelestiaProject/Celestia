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

#include <vector>

#include <Eigen/Core>

namespace cmod
{
class Mesh;
}

struct SphereMeshParameters
{
    Eigen::Vector3f size;
    Eigen::Vector3f offset;
    float featureHeight;
    float octaves;
    float slices;
    float rings;

    float value(float u, float v) const;
};

/*! The SphereMesh class is used to generate displacement mapped
 *  spheres when loading the now-deprecated .cms geometry files.
 */
class SphereMesh
{
public:
    SphereMesh(const Eigen::Vector3f& size,
               int _nRings, int _nSlices,
               const SphereMeshParameters& params);
    ~SphereMesh() = default;

    //! Convert this object into a standard Celestia mesh.
    cmod::Mesh convertToMesh() const;

 private:
    void createSphere();
    void generateNormals();
    void scale(const Eigen::Vector3f&);
    void fixNormals();
    void displace(const SphereMeshParameters& params);

    int nRings;
    int nSlices;
    int nVertices;
    std::vector<Eigen::Vector3f> vertices{ };
    std::vector<Eigen::Vector3f> normals{ };
    std::vector<Eigen::Vector2f> texCoords{ };
};
