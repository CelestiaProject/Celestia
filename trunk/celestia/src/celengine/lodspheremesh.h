// lodspheremesh.h
// 
// Copyright (C) 2001-2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef CELENGINE_LODSPHEREMESH_H_
#define CELENGINE_LODSPHEREMESH_H_

#include <celengine/texture.h>
#include <celengine/glcontext.h>
#include <celmath/vecmath.h>
#include <celmath/frustum.h>


#define MAX_SPHERE_MESH_TEXTURES 6
#define NUM_SPHERE_VERTEX_BUFFERS 2

class LODSphereMesh
{
public:
    LODSphereMesh();
    ~LODSphereMesh();

    void render(const GLContext&,
                unsigned int attributes, const Frustum&, float pixWidth,
                Texture** tex, int nTextures);
    void render(const GLContext&,
                unsigned int attributes, const Frustum&, float pixWidth,
                Texture* tex0 = NULL, Texture* tex1 = NULL,
                Texture* tex2 = NULL, Texture* tex3 = NULL);
    void render(const GLContext&,
                const Frustum&, float pixWidth,
                Texture** tex, int nTextures);

    enum {
        Normals    = 0x01,
        Tangents   = 0x02,
        Colors     = 0x04,
        TexCoords0 = 0x08,
        TexCoords1 = 0x10,
        VertexProgParams = 0x1000,
        Multipass  = 0x10000000,
    };

 private:
    struct RenderInfo
    {
        RenderInfo(int _step,
                   unsigned int _attr,
                   const Frustum& _frustum,
                   const GLContext& _context) :
            step(_step),
            attributes(_attr),
            frustum(_frustum),
            context(_context)
        {};

        int step;                 
        unsigned int attributes;  // vertex attributes
        const Frustum& frustum;   // frustum, for culling
        Point3f fp[8];            // frustum points, for culling
        int texLOD[MAX_SPHERE_MESH_TEXTURES];
        const GLContext& context;
    };

    int renderPatches(int phi0, int theta0, 
                      int extent,
                      int level,
                      const RenderInfo&);

    void renderSection(int phi0, int theta0, int extent, const RenderInfo&);

    float* vertices;

    int maxVertices;
    int vertexSize;

    int nIndices;
    unsigned short* indices;

    int nTexturesUsed;
    Texture* textures[MAX_SPHERE_MESH_TEXTURES];
    unsigned int subtextures[MAX_SPHERE_MESH_TEXTURES];

    bool vertexBuffersInitialized;
    bool useVertexBuffers;
    int currentVB;
    unsigned int vertexBuffers[NUM_SPHERE_VERTEX_BUFFERS];
    GLuint indexBuffer;
};

#endif // CELENGINE_LODSPHEREMESH_H_
