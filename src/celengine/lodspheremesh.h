// lodspheremesh.h
//
// Copyright (C) 2001-present, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include <Eigen/Core>

#include <celengine/glsupport.h>

class Texture;

namespace celestia::math
{
class Frustum;
}

class CelestiaGLProgram;

class LODSphereMesh
{
public:
    static constexpr std::size_t MAX_SPHERE_MESH_TEXTURES = 6;
    static constexpr std::size_t NUM_SPHERE_VERTEX_BUFFERS = 2;

    LODSphereMesh() = default;
    ~LODSphereMesh();

    LODSphereMesh(const LODSphereMesh&) = delete;
    LODSphereMesh& operator=(const LODSphereMesh&) = delete;
    LODSphereMesh(LODSphereMesh&&) = delete;
    LODSphereMesh& operator=(LODSphereMesh&&) = delete;

    void render(unsigned int attributes, const celestia::math::Frustum&, float pixWidth,
                Texture** tex, int nTextures, CelestiaGLProgram *);
    void render(unsigned int attributes, const celestia::math::Frustum&, float pixWidth, CelestiaGLProgram *,
                Texture* tex0 = nullptr, Texture* tex1 = nullptr,
                Texture* tex2 = nullptr, Texture* tex3 = nullptr);
    void render(const celestia::math::Frustum&, float pixWidth,
                Texture** tex, int nTextures, CelestiaGLProgram *);

    enum
    {
        Normals    = 0x01,
        Tangents   = 0x02,
    };

 private:
    struct RenderInfo
    {
        RenderInfo(int _step,
                   unsigned int _attr,
                   const celestia::math::Frustum& _frustum) :
            step(_step),
            attributes(_attr),
            frustum(_frustum)
        {};

        int step;
        unsigned int attributes;  // vertex attributes
        const celestia::math::Frustum& frustum;   // frustum, for culling
        std::array<Eigen::Vector3f, 8> fp{};    // frustum points, for culling
        std::array<int, MAX_SPHERE_MESH_TEXTURES> texLOD{};
    };

    void renderPatches(int phi0, int theta0,
                       int extent,
                       int level,
                       const RenderInfo&,
                       CelestiaGLProgram *);

    void renderSection(int phi0, int theta0, int extent, const RenderInfo&, CelestiaGLProgram *);

    int vertexSize{ 0 };

    std::vector<float> vertices{};
    std::vector<unsigned short> indices{};

    int nTexturesUsed{ 0 };
    std::array<Texture*, MAX_SPHERE_MESH_TEXTURES> textures{};
    std::array<unsigned int, MAX_SPHERE_MESH_TEXTURES> subtextures{};

    bool vertexBuffersInitialized{ false };
    GLuint currentVB{ 0 };
    std::array<GLuint, NUM_SPHERE_VERTEX_BUFFERS> vertexBuffers{};
    GLuint indexBuffer{ 0 };
};
