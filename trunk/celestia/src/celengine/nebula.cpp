// nebula.cpp
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <stdio.h>
#include "celestia.h"
#include <celmath/mathlib.h>
#include <celutil/util.h>
#include <celutil/debug.h>
#include "astro.h"
#include "nebula.h"
#include "meshmanager.h"
#include "gl.h"
#include "vecgl.h"

using namespace std;


Nebula::Nebula() :
    mesh(InvalidResource)
{
}


ResourceHandle Nebula::getMesh() const
{
    return mesh;
}

void Nebula::setMesh(ResourceHandle _mesh)
{
    mesh = _mesh;
}


bool Nebula::load(AssociativeArray* params)
{
    string mesh;
    if (params->getString("Mesh", mesh))
    {
        ResourceHandle meshHandle = GetMeshManager()->getHandle(MeshInfo(mesh));
        setMesh(meshHandle);
    }
    
    return DeepSkyObject::load(params);
}


void Nebula::render(const Vec3f& offset,
                    const Quatf& viewerOrientation,
                    float brightness,
                    float pixelSize)
{
    Mesh* m = NULL;
    if (mesh != InvalidResource)
        m = GetMeshManager()->find(mesh);
    if (m == NULL)
        return;

    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glScalef(getRadius(), getRadius(), getRadius());

    m->render(Mesh::TexCoords0, 0);
}
