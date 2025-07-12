// referencemarkrenderer.h
//
// Copyright (C) 2025, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>
#include <vector>

#include "linerenderer.h"

class CelestiaGLProgram;

namespace celestia
{

namespace gl
{
class Buffer;
class VertexObject;
}

namespace render
{

class ArrowRenderer
{
public:
    explicit ArrowRenderer(const Renderer& renderer);
    ~ArrowRenderer();

    CelestiaGLProgram* program() const { return m_prog; }
    gl::VertexObject* vertexObject() const { return m_vo.get(); }
    LineRenderer& lineRenderer() { return m_lineRenderer; }

private:
    CelestiaGLProgram* m_prog;
    std::unique_ptr<gl::Buffer> m_buffer;
    std::unique_ptr<gl::VertexObject> m_vo;
    LineRenderer m_lineRenderer;
};

class PlanetGridRenderer
{
public:
    explicit PlanetGridRenderer(const Renderer& renderer);

    LineRenderer& latitudeRenderer() { return m_latitudeRenderer; }
    LineRenderer& equatorRenderer() { return m_equatorRenderer; }
    LineRenderer& longitudeRenderer() { return m_longitudeRenderer; }

private:
    LineRenderer m_latitudeRenderer;
    LineRenderer m_equatorRenderer;
    LineRenderer m_longitudeRenderer;
};

class ReferenceMarkRenderer
{
public:
    explicit ReferenceMarkRenderer(Renderer&);

    Renderer& renderer() { return m_renderer; }
    ArrowRenderer& arrowRenderer();
    PlanetGridRenderer& planetGridRenderer();
    LineRenderer& visibleRegionRenderer();

private:
    Renderer& m_renderer;
    std::unique_ptr<ArrowRenderer> m_arrowRenderer;
    std::unique_ptr<PlanetGridRenderer> m_planetGridRenderer;
    std::unique_ptr<LineRenderer> m_visibleRegionRenderer;
};

} // end namespace celestia::render

} // end namespace celestia
