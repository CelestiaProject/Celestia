// 3dsmesh.h
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _3DSMESH_H_
#define _3DSMESH_H_

#include <vector>
#include <celengine/mesh.h>
#include <celengine/vertexlist.h>
#include <cel3ds/3dsmodel.h>

class Mesh3DS : public Mesh
{
 public:
    Mesh3DS(const M3DScene& scene);
    ~Mesh3DS();

    void render(float lod);
    void render(unsigned int attributes, float lod);
    void render(unsigned int attributes, const Frustum&, float lod);
    void normalize();

 private:
    typedef std::vector<VertexList*> VertexListVec;
    VertexListVec vertexLists;
};

#endif // _3DSMESH_H_
