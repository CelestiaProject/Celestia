// cometrenderer.h
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <utility>
#include <Eigen/Core>

class Body;
class Observer;
class Renderer;
class CelestiaGLProgram;
struct Matrices;

namespace celestia::render
{
class IndexedVertexObject;

class CometRenderer
{
public:
    explicit CometRenderer(Renderer &renderer);
    ~CometRenderer() = default;
    CometRenderer(const CometRenderer&) = delete;
    CometRenderer(CometRenderer&&) = delete;
    CometRenderer& operator=(const CometRenderer&) = delete;
    CometRenderer& operator=(CometRenderer&&) = delete;

    void render(const Body &body,
                const Observer &observer,
                const Eigen::Vector3f &pos,
                float dustTailLength,
                float discSizeInPixels,
                const Matrices &m);

    void initGL();
    void deinitGL();

private:

    void initVertexObject();

    struct CometTailVertex
    {
        Eigen::Vector3f point;
        Eigen::Vector3f normal;
        float brightness;
    };

    Renderer                               &m_renderer;
    CelestiaGLProgram                      *m_prog              { nullptr };
    int                                     m_brightnessLoc     { -1 };
    bool                                    m_initialized       { false };

    std::unique_ptr<CometTailVertex[]>      m_vertices;
    std::unique_ptr<unsigned short[]>       m_indices;
    std::unique_ptr<IndexedVertexObject>    m_vo;
};
}
