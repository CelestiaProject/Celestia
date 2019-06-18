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
#ifdef USE_GLCONTEXT
#include <celengine/glcontext.h>
#endif
#include <Eigen/Geometry>
#include <celmath/frustum.h>


#define MAX_SPHERE_MESH_TEXTURES 6
#define NUM_SPHERE_VERTEX_BUFFERS 2

class LODSphereMesh
{
public:
    LODSphereMesh();
    ~LODSphereMesh();

    void render(unsigned int attributes, const celmath::Frustum&, float pixWidth,
                Texture** tex, int nTextures);
    void render(unsigned int attributes, const celmath::Frustum&, float pixWidth,
                Texture* tex0 = nullptr, Texture* tex1 = nullptr,
                Texture* tex2 = nullptr, Texture* tex3 = nullptr);
    void render(const celmath::Frustum&, float pixWidth,
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
                   const celmath::Frustum& _frustum) :
            step(_step),
            attributes(_attr),
            frustum(_frustum)
        {};

        int step;
        unsigned int attributes;  // vertex attributes
        const celmath::Frustum& frustum;   // frustum, for culling
        Eigen::Vector3f fp[8];    // frustum points, for culling
        int texLOD[MAX_SPHERE_MESH_TEXTURES];
    };

    int renderPatches(int phi0, int theta0,
                      int extent,
                      int level,
                      const RenderInfo&);

    void renderSection(int phi0, int theta0, int extent, const RenderInfo&);

    float* vertices{ nullptr };

    int maxVertices{ 0 };
    int vertexSize{ 0 };

    int nIndices{ 0 };
    unsigned short* indices{ nullptr };

    int nTexturesUsed{ 0 };
    Texture* textures[MAX_SPHERE_MESH_TEXTURES]{};
    unsigned int subtextures[MAX_SPHERE_MESH_TEXTURES]{};

    bool vertexBuffersInitialized{ false };
    bool useVertexBuffers{ false };
    int currentVB{ 0 };
    unsigned int vertexBuffers[NUM_SPHERE_VERTEX_BUFFERS];
    GLuint indexBuffer{ 0 };
};

#endif // CELENGINE_LODSPHEREMESH_H_
