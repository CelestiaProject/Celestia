// 3dsmesh.h
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_3DSMESH_H_
#define _CELENGINE_3DSMESH_H_

#include <vector>
#include <string>
#include <celmath/frustum.h>
#include <celengine/mesh.h>
#include <celengine/vertexlist.h>
#include <cel3ds/3dsmodel.h>

class Mesh3DS : public Mesh
{
 public:
    Mesh3DS(const M3DScene& scene, const std::string& texturePath);
    ~Mesh3DS();

    void render(float lod);
    void render(unsigned int attributes, float lod);
    void render(unsigned int attributes, const Frustum&, float lod);
    bool pick(const Ray3d& ray, double& distance);
    void normalize(const Vec3f& centerOffset);

 private:
    typedef std::vector<VertexList*> VertexListVec;
    VertexListVec vertexLists;
};

#endif // _CELENGINE_3DSMESH_H_
