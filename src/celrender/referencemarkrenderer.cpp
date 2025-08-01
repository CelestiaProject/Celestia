// referencemarkrenderer.cpp
//
// Copyright (C) 2007-2025, Celestia Development Team
//
// Based on axisarrow.cpp, planetgrid.cpp
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "referencemarkrenderer.h"

#include <Eigen/Core>

#include <celcompat/numbers.h>
#include <celengine/glsupport.h>
#include <celengine/render.h>
#include <celengine/shadermanager.h>
#include <celmath/mathlib.h>
#include "gl/buffer.h"
#include "gl/vertexobject.h"

namespace celestia::render
{

namespace
{

constexpr unsigned int arrowSections = 30U;

std::vector<Eigen::Vector3f>
getArrowVertices()
{
    constexpr float shaftLength  = 0.85f;
    constexpr float headLength   = 0.10f;
    constexpr float shaftRadius  = 0.010f;
    constexpr float headRadius   = 0.025f;

    constexpr unsigned int totalVertices = arrowSections * 3U + 3U;

    std::vector<Eigen::Vector3f> vertices;
    vertices.reserve(totalVertices);
    vertices.emplace_back(Eigen::Vector3f::Zero());
    vertices.emplace_back(0.0f, 0.0f, shaftLength);
    vertices.emplace_back(0.0f, 0.0f, shaftLength + headLength);

    for (unsigned int i = 0U; i < arrowSections; ++i)
    {
        float s;
        float c;
        math::sincos((static_cast<float>(i) * 2.0f * celestia::numbers::pi_v<float>) / arrowSections, s, c);

        vertices.emplace_back(shaftRadius * s, shaftRadius * c, 0.0f);
        vertices.emplace_back(shaftRadius * s, shaftRadius * c, shaftLength);
        vertices.emplace_back(headRadius * s, headRadius * c, shaftLength);
    }

    return vertices;
}

std::vector<GLushort>
getArrowIndices()
{
    constexpr unsigned int circleSize = (arrowSections + 1U) * 3U;
    constexpr unsigned int shaftSize = 3U + arrowSections * 6U;
    constexpr unsigned int annulusSize = (arrowSections + 1U) * 3U;
    constexpr unsigned int headSize = (arrowSections + 1U) * 3U;

    constexpr unsigned int totalSize = circleSize + shaftSize + annulusSize + headSize;

    std::vector<GLushort> indices(totalSize, GLushort(0));

    GLushort* const circle = indices.data();
    GLushort* const shaft = circle + circleSize;
    GLushort* const annulus = shaft + shaftSize;
    GLushort* const head = annulus + annulusSize;

    GLushort* circlePtr = circle;
    GLushort* shaftPtr = shaft;
    GLushort* annulusPtr = annulus;
    GLushort* headPtr = head;

    constexpr GLushort vZero = 0;
    constexpr GLushort v3 = 1;
    constexpr GLushort v4 = 2;
    for (unsigned int i = 0U; i <= arrowSections; ++i)
    {
        unsigned int idx = i < arrowSections ? i : 0;
        auto v0 = static_cast<GLushort>(3U + idx * 3U);
        auto v1 = static_cast<GLushort>(v0 + 1U);
        auto v2 = static_cast<GLushort>(v0 + 2U);

        // Circle at bottom
        if (i > 0)
            *(circlePtr++) = v0;
        *(circlePtr++) = vZero;
        *(circlePtr++) = v1;

        // Shaft
        if (i > 0)
        {
            *(shaftPtr++) = v0; // left triangle

            *(shaftPtr++) = v0; // right
            *(shaftPtr++) = static_cast<GLushort>(idx * 3U + 1U) ; // v1Prev
            *(shaftPtr++) = v1;
        }
        *(shaftPtr++) = v0; // left
        *(shaftPtr++) = v1;

        // Annulus
        if (i > 0)
            *(annulusPtr++) = v2;
        *(annulusPtr++) = v2;
        *(annulusPtr++) = v3;

        // Head
        if (i > 0)
            *(headPtr++) = v2;
        *(headPtr++) = v4;
        *(headPtr++) = v2;
    }

    *circlePtr = circle[1];
    *shaftPtr = shaft[0];
    *annulusPtr = annulus[1];
    *headPtr = head[1];

    return indices;
}

constexpr unsigned int circleSubdivisions = 100U;

} // end unnamed namespace

ArrowRenderer::~ArrowRenderer() = default;

ArrowRenderer::ArrowRenderer(const Renderer& renderer) :
    m_vo(std::make_unique<gl::VertexObject>()), //NOSONAR
    m_lineRenderer(renderer)
{
    auto vertices = getArrowVertices();
    auto indices = getArrowIndices();

    m_buffer = std::make_unique<gl::Buffer>(gl::Buffer::TargetHint::Array);
    m_buffer->setData(vertices, gl::Buffer::BufferUsage::StaticDraw);

    m_vo->addVertexBuffer(*m_buffer,
                          CelestiaGLProgram::VertexCoordAttributeIndex,
                          3,
                          gl::VertexObject::DataType::Float);
    gl::Buffer indexBuffer(gl::Buffer::TargetHint::ElementArray, indices);
    m_vo->setCount(static_cast<int>(indices.size()));
    m_vo->setIndexBuffer(std::move(indexBuffer), 0, gl::VertexObject::IndexType::UnsignedShort);

    ShaderProperties shaderProperties;
    shaderProperties.texUsage = TexUsage::VertexColors;
    shaderProperties.lightModel = LightingModel::UnlitModel;
    m_prog = renderer.getShaderManager().getShader(shaderProperties);
}

PlanetGridRenderer::PlanetGridRenderer(const Renderer& renderer) :
    m_latitudeRenderer(renderer, 1.0f, LineRenderer::PrimType::LineStrip, LineRenderer::StorageType::Static),
    m_equatorRenderer(renderer, 2.0f, LineRenderer::PrimType::LineStrip, LineRenderer::StorageType::Static),
    m_longitudeRenderer(renderer, 1.0f, LineRenderer::PrimType::LineStrip, LineRenderer::StorageType::Static)
{
    for (unsigned int i = 0; i <= circleSubdivisions + 1; i++)
    {
        float s, c;
        math::sincos((2.0f * celestia::numbers::pi_v<float>) * static_cast<float>(i) / circleSubdivisions, s, c);
        Eigen::Vector3f latitudePoint(c, 0.0f, s);
        Eigen::Vector3f longitudePoint(c, s, 0.0f);
        m_latitudeRenderer.addVertex(latitudePoint);
        m_equatorRenderer.addVertex(latitudePoint);
        m_longitudeRenderer.addVertex(longitudePoint);
    }
}

ReferenceMarkRenderer::ReferenceMarkRenderer(Renderer& renderer) :
    m_renderer(renderer)
{
}

ArrowRenderer&
ReferenceMarkRenderer::arrowRenderer()
{
    if (!m_arrowRenderer)
        m_arrowRenderer = std::make_unique<ArrowRenderer>(m_renderer);
    return *m_arrowRenderer;
}

PlanetGridRenderer&
ReferenceMarkRenderer::planetGridRenderer()
{
    if (!m_planetGridRenderer)
        m_planetGridRenderer = std::make_unique<PlanetGridRenderer>(m_renderer);
    return *m_planetGridRenderer;
}

LineRenderer&
ReferenceMarkRenderer::visibleRegionRenderer()
{
    if (!m_visibleRegionRenderer)
        m_visibleRegionRenderer = std::make_unique<LineRenderer>(m_renderer, 1.0f, render::LineRenderer::PrimType::LineStrip);
    return *m_visibleRegionRenderer;
}

} // end namespace celestia::render
