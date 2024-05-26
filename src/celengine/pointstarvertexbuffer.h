// pointstarvertexbuffer.h
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>

#include <Eigen/Core>

#include <celutil/color.h>

class Renderer;
class Texture;
class CelestiaGLProgram;

namespace celestia::gl
{
class Buffer;
class VertexObject;
}

// PointStarVertexBuffer is used when hardware supports point sprites.
class PointStarVertexBuffer
{
public:
    using capacity_type = unsigned int;

    PointStarVertexBuffer(const Renderer &renderer, capacity_type capacity);
    ~PointStarVertexBuffer() = default;
    PointStarVertexBuffer() = delete;
    PointStarVertexBuffer(const PointStarVertexBuffer&) = delete;
    PointStarVertexBuffer(PointStarVertexBuffer&&) = delete;
    PointStarVertexBuffer& operator=(const PointStarVertexBuffer&) = delete;
    PointStarVertexBuffer& operator=(PointStarVertexBuffer&&) = delete;

    void startBasicPoints();
    void startSprites();
    void render();
    void finish();
    void addStar(const Eigen::Vector3f &pos, const Color &color, float size);
    void setTexture(Texture* texture);
    void setPointScale(float);

    static void enable();
    static void disable();

private:
    struct StarVertex
    {
        Eigen::Vector3f position;
        float size;
        unsigned char color[4];
    };

    const Renderer                 &m_renderer;
    capacity_type                   m_capacity;
    capacity_type                   m_nStars                { 0 };
    std::unique_ptr<StarVertex[]>   m_vertices;
    Texture                        *m_texture               { nullptr };
    bool                            m_pointSizeFromVertex   { false };
    float                           m_pointScale            { 1.0f };
    CelestiaGLProgram              *m_prog                  { nullptr };

    std::unique_ptr<celestia::gl::Buffer>        m_bo;
    std::unique_ptr<celestia::gl::VertexObject>  m_vo1;
    std::unique_ptr<celestia::gl::VertexObject>  m_vo2;
    bool m_initialized{ false };

    static PointStarVertexBuffer    *current;

    void makeCurrent();
    void setupVertexArrayObject();
};

inline void
PointStarVertexBuffer::addStar(const Eigen::Vector3f &pos,
                               const Color &color,
                               float size)
{
    if (m_nStars < m_capacity)
    {
        m_vertices[m_nStars].position = pos;
        m_vertices[m_nStars].size = size;
        color.get(m_vertices[m_nStars].color);
        m_nStars++;
    }

    if (m_nStars == m_capacity)
    {
        render();
        m_nStars = 0;
    }
}
