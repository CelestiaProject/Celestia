// model.cpp
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "model.h"
#include <cassert>

using namespace std;


#if 0
static GLenum GLPrimitiveModes[MaxPrimitiveType] = 
{
    GL_TRIANGLES,
    GL_TRIANGLE_STRIP,
    GL_TRIANGLE_FAN,
    GL_LINES,
    GL_LINE_STRIP,
    GL_POINTS
};
#endif

static size_t VertexAttributeFormatSizes[Mesh::FormatMax] = 
{
     4,  // Float1
     8,  // Float2
     12, // Float3
     16, // Float4,
     4,  // UByte4
};


Model::Model()
{
}


Model::~Model()
{
    {
        for (vector<const Mesh*>::iterator iter = meshes.begin();
             iter != meshes.end(); iter++)
            delete *iter;
    }

#if 0
    {
        for (vector<const Mesh::Material*>::iterator iter = materials.begin();
             iter != materials.end(); iter++)
            delete *iter;
    }
#endif
}


const Mesh::Material*
Model::getMaterial(uint32 index) const
{
    return materials[index];
}


uint32
Model::addMaterial(const Mesh::Material* m)
{
    materials.push_back(m);
    return materials.size();
}


const Mesh*
Model::getMesh(uint32 index) const
{
    return meshes[index];
}


uint32
Model::addMesh(const Mesh* m)
{
    meshes.push_back(m);
    return meshes.size();
}


bool
Model::pick(const Ray3d& r, double& distance) const
{
    double maxDistance = 1.0e30;
    double closest = maxDistance;

    for (vector<const Mesh*>::const_iterator iter = meshes.begin();
         iter != meshes.end(); iter++)
    {
        double d = maxDistance;
        if ((*iter)->pick(r, d) && d < closest)
            closest = d;
    }    

    if (closest != maxDistance)
    {
        distance = closest;
        return true;
    }
    else
    {
        return false;
    }
}


void
Model::render()
{
    for (vector<const Mesh*>::const_iterator iter = meshes.begin();
         iter != meshes.end(); iter++)
    {
        (*iter)->render(materials);
    }

}

