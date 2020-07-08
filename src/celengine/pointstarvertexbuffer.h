// pointstarvertexbuffer.h
//
// Copyright (C) 2001-2019, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <Eigen/Core>
#include "glshader.h"

class Color;
class Renderer;
class Texture;

// PointStarVertexBuffer is used when hardware supports point sprites.
class PointStarVertexBuffer
{
public:
    using capacity_t = unsigned int;

    PointStarVertexBuffer(const Renderer& _renderer, unsigned int _capacity);
    ~PointStarVertexBuffer();
    PointStarVertexBuffer() = delete;
    PointStarVertexBuffer(const PointStarVertexBuffer&) = delete;
    PointStarVertexBuffer(PointStarVertexBuffer&&) = delete;
    PointStarVertexBuffer& operator=(const PointStarVertexBuffer&) = delete;
    PointStarVertexBuffer& operator=(PointStarVertexBuffer&&) = delete;

    void startPoints();
    void startSprites();
    void render();
    void finish();
    inline void addStar(const Eigen::Vector3f& pos, const Color&, float);
    void setTexture(Texture* /*_texture*/);

private:
    struct StarVertex
    {
        Eigen::Vector3f position;
        float size;
        unsigned char color[4];
        float pad;
    };

    const Renderer& renderer;
    capacity_t capacity;

    capacity_t nStars       { 0 };
    StarVertex* vertices    { nullptr };
    Texture* texture        { nullptr };
    bool useSprites         { false };
    IntegerShaderParameter starTexParam;
};

inline void PointStarVertexBuffer::addStar(const Eigen::Vector3f& pos,
                                           const Color& color,
                                           float size)
{
    if (nStars < capacity)
    {
        vertices[nStars].position = pos;
        vertices[nStars].size = size;
        color.get(vertices[nStars].color);
        nStars++;
    }

    if (nStars == capacity)
    {
        render();
        nStars = 0;
    }
}
