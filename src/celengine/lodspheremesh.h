// lodspheremesh.h
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef CELENGINE_LODSPHEREMESH_H_
#define CELENGINE_LODSPHEREMESH_H_

#include <celmath/vecmath.h>
#include <celengine/mesh.h>
#include <celengine/texture.h>
#include <celengine/glcontext.h>


#define MAX_SPHERE_MESH_TEXTURES 4


class LODSphereMesh
{
public:
    LODSphereMesh();
    ~LODSphereMesh();

    void render(const GLContext&,
                unsigned int attributes, const Frustum&, float lod,
                Texture** tex, int nTextures);
    void render(const GLContext&,
                unsigned int attributes, const Frustum&, float lod,
                Texture* tex0 = NULL, Texture* tex1 = NULL,
                Texture* tex2 = NULL, Texture* tex3 = NULL);
    void render(const GLContext&,
                const Frustum&, float lod,
                Texture** tex, int nTextures);

 private:
    int renderPatches(int phi0, int theta0, 
                      int extent,
                      int level,
                      int step,
                      unsigned int attributes,
                      Point3f* fp);

    void renderSection(int phi0, int theta0,
                       int extent,
                       int step,
                       unsigned int attributes);

    float* vertices;
    float* normals;
    float* texCoords[MAX_SPHERE_MESH_TEXTURES];
    float* tangents;
    int nIndices;
    unsigned short* indices;

    int nTexturesUsed;
    Texture* textures[MAX_SPHERE_MESH_TEXTURES];
    unsigned int subtextures[MAX_SPHERE_MESH_TEXTURES];
};

#endif // CELENGINE_LODSPHEREMESH_H_
