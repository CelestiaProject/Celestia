// model.cpp
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "model.h"
#include "rendcontext.h"
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
        for (vector<Mesh*>::iterator iter = meshes.begin();
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
    if (index < materials.size())
        return materials[index];
    else
        return NULL;
}


uint32
Model::addMaterial(const Mesh::Material* m)
{
    materials.push_back(m);
    return materials.size();
}


Mesh*
Model::getMesh(uint32 index) const
{
    if (index < meshes.size())
        return meshes[index];
    else
        return NULL;
}


uint32
Model::addMesh(Mesh* m)
{
    meshes.push_back(m);
    return meshes.size();
}


bool
Model::pick(const Ray3d& r, double& distance) const
{
    double maxDistance = 1.0e30;
    double closest = maxDistance;

    for (vector<Mesh*>::const_iterator iter = meshes.begin();
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
Model::render(RenderContext& rc)
{
    for (vector<Mesh*>::const_iterator iter = meshes.begin();
         iter != meshes.end(); iter++)
    {
        (*iter)->render(materials, rc);
    }
}


void
Model::normalize(const Vec3f& centerOffset)
{
    AxisAlignedBox bbox;

    vector<Mesh*>::const_iterator iter;
    for (iter = meshes.begin(); iter != meshes.end(); iter++)
        bbox.include((*iter)->getBoundingBox());

    Point3f center = bbox.getCenter() + centerOffset;
    Vec3f extents = bbox.getExtents();
    float maxExtent = extents.x;
    if (extents.y > maxExtent)
        maxExtent = extents.y;
    if (extents.z > maxExtent)
        maxExtent = extents.z;

    for (iter = meshes.begin(); iter != meshes.end(); iter++)
        (*iter)->transform(Point3f(0, 0, 0) - center, 2.0f / maxExtent);

#if 0
    for (i = vertexLists.begin(); i != vertexLists.end(); i++)
        (*i)->transform(Point3f(0, 0, 0) - center, 2.0f / maxExtent);
#endif
}
