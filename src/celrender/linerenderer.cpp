// linerenderer.h
//
// Copyright (C) 2022-present, Celestia Development Team.
//
// Line renderer.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celengine/glsupport.h>        // gl::maxLineWidth
#include <celengine/shadermanager.h>
#include <celengine/render.h>
#include "linerenderer.h"
#include <iostream>

namespace celestia::engine
{
void
LineRenderer::draw_triangles(int count, int offset)
{
    assert((count + offset) <= static_cast<int>(m_segments.size()));

    m_trVertexObj->draw(GL_TRIANGLES, count, offset);
    //m_trVertexObj->orphan();
    m_trVertexObj->unbind();
}

void
LineRenderer::draw_triangle_strip(int count, int offset)
{
    assert((count + offset) <= static_cast<int>(m_verticesTr.size()));

    m_trVertexObj->draw(GL_TRIANGLE_STRIP, count, offset);
    //m_trVertexObj->orphan();
    m_trVertexObj->unbind();
}

void
LineRenderer::draw_lines(int count, int offset)
{
    assert(count + offset <= static_cast<int>(m_vertices.size()));

    m_lnVertexObj->draw(static_cast<GLenum>(m_primType), count, offset);
    //m_lnVertexObj->orphan();
    m_lnVertexObj->unbind();
}

void
LineRenderer::setup_shader(const Eigen::Matrix4f &projection, const Eigen::Matrix4f &modelview)
{
    if (m_prog == nullptr)
    {
        ShaderProperties props;
        props.texUsage = ShaderProperties::VertexColors;
        props.lightModel = ShaderProperties::UnlitModel;
        if (m_useTriangles)
            props.texUsage |= ShaderProperties::LineAsTriangles;
        if ((m_hints & DISABLE_FISHEYE_TRANFORMATION) != 0)
            props.fishEyeOverride = ShaderProperties::FisheyeOverrideModeDisabled;
        m_prog = m_renderer.getShaderManager().getShader(props);
        if (m_prog == nullptr)
            return;
    }

    m_prog->use();
    m_prog->setMVPMatrices(projection, modelview);
    if (m_useTriangles)
    {
        m_prog->lineWidthX = m_width * m_renderer.getLineWidthX();
        m_prog->lineWidthY = m_width * m_renderer.getLineWidthY();
    }
    else
    {
        glLineWidth(m_renderer.getRasterizedLineWidth(m_width));
    }
}

void
LineRenderer::setup_vbo()
{
    if (!m_useTriangles)
    {
        if (m_lnVertexObj != nullptr)
        {
            if (m_storageType == STATIC_STORAGE)
            {
                m_lnVertexObj->bind();
            }
            else
            {
                m_lnVertexObj->bindWritable();
                m_lnVertexObj->allocate(m_vertices.capacity() * sizeof(m_vertices[0]), nullptr); // orphan old buffer
                m_lnVertexObj->setBufferData(m_vertices.data());
            }
        }
        else
        {
            auto storageType = static_cast<GLenum>(m_storageType);
            m_lnVertexObj = std::make_unique<celgl::VertexObject>(GL_ARRAY_BUFFER, 0, storageType);
            m_lnVertexObj->bind();

            m_lnVertexObj->allocate(m_vertices.capacity() * sizeof(m_vertices[0]), m_vertices.data());
            m_lnVertexObj->setVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex,
                                                4, GL_FLOAT, GL_FALSE, 0, 0);
        }
    }
    else
    {
        if (m_trVertexObj != nullptr)
        {
            if (m_storageType == STATIC_STORAGE)
            {
                m_trVertexObj->bind();
            }
            else
            {
                m_trVertexObj->bindWritable();
                if (m_primType == LINES || (m_hints & PREFER_SIMPLE_TRIANGLES) != 0)
                {
                    auto stride = static_cast<GLsizei>(sizeof(LineSegment));
                    m_trVertexObj->allocate(m_segments.capacity() * stride, nullptr); // orphan old buffer
                    m_trVertexObj->setBufferData(m_segments.data());
                }
                else
                {
                    auto stride = static_cast<GLsizei>(sizeof(LineVertex));
                    m_trVertexObj->allocate(m_verticesTr.capacity() * stride, nullptr); // orphan old buffer
                    m_trVertexObj->setBufferData(m_verticesTr.data());
                }
            }
        }
        else
        {
            auto storageType = static_cast<GLenum>(m_storageType);
            m_trVertexObj = std::make_unique<celgl::VertexObject>(GL_ARRAY_BUFFER, 0, storageType);
            m_trVertexObj->bind();

            if (m_primType == LINES || (m_hints & PREFER_SIMPLE_TRIANGLES) != 0)
            {
                auto stride = static_cast<GLsizei>(sizeof(LineSegment));

                m_trVertexObj->allocate(m_segments.capacity() * stride, m_segments.data());
                m_trVertexObj->setVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex,
                                                    4, GL_FLOAT, GL_FALSE, stride, offsetof(LineSegment, point1));
                m_trVertexObj->setVertexAttribArray(CelestiaGLProgram::NextVCoordAttributeIndex,
                                                    4, GL_FLOAT, GL_FALSE, stride, offsetof(LineSegment, point2));
                m_trVertexObj->setVertexAttribArray(CelestiaGLProgram::ScaleFactorAttributeIndex,
                                                    1, GL_FLOAT, GL_FALSE, stride, offsetof(LineSegment, scale));
                m_segments.clear();
            }
            else
            {
                auto stride = static_cast<GLsizei>(sizeof(LineVertex));

                m_trVertexObj->allocate(m_verticesTr.capacity() * stride, m_verticesTr.data());
                m_trVertexObj->setVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex,
                                                    4, GL_FLOAT, GL_FALSE, stride, offsetof(LineVertex, point));
                m_trVertexObj->setVertexAttribArray(CelestiaGLProgram::NextVCoordAttributeIndex,
                                                    4, GL_FLOAT, GL_FALSE, stride, 2*stride + offsetof(LineVertex, point));
                m_trVertexObj->setVertexAttribArray(CelestiaGLProgram::ScaleFactorAttributeIndex,
                                                    1, GL_FLOAT, GL_FALSE, stride, offsetof(LineVertex, scale));
                m_verticesTr.clear();
            }
        }
    }
}

void
LineRenderer::add_segment_points(const Eigen::Vector4f &point1, const Eigen::Vector4f &point2)
{
    m_segments.emplace_back(point1, point2, -0.5f);
    m_segments.emplace_back(point1, point2,  0.5f);
    m_segments.emplace_back(point2, point1, -0.5f);
    m_segments.emplace_back(point2, point1, -0.5f);
    m_segments.emplace_back(point2, point1,  0.5f);
    m_segments.emplace_back(point1, point2, -0.5f);
}

void
LineRenderer::triangulate_segments()
{
    int count = static_cast<int>(m_vertices.size());
    m_segments.reserve(count*3);
    for (int i = 0; i < count; i+=2)
        add_segment_points(m_vertices[i], m_vertices[i+1]);
}

void
LineRenderer::triangulate_vertices_as_segments()
{
    int count = static_cast<int>(m_vertices.size());
    int stop = m_primType == LINE_STRIP ? count-1 : count;
    m_segments.reserve(stop*6);
    for (int i = 0; i < stop; i++)
        add_segment_points(m_vertices[i], m_vertices[(i + 1) % count]);
}

void
LineRenderer::close_loop()
{
    // simulate loop by adding an additional endpoint (copy of vertex[0])
    // vertex[1] is required as a next position to calculate triangle vertices
    m_verticesTr.emplace_back(m_vertices[0], -0.5f);
    m_verticesTr.emplace_back(m_vertices[0],  0.5f);
    m_verticesTr.emplace_back(m_vertices[1], -0.5f);
    m_verticesTr.emplace_back(m_vertices[1],  0.5f);
    m_loopDone = true;
}

void
LineRenderer::triangulate_vertices()
{
    m_verticesTr.reserve(m_vertices.size()*2 + (m_primType == LINE_LOOP ? 4 : 0));
    for (const auto &vertex : m_vertices)
    {
        m_verticesTr.emplace_back(vertex, -0.5f);
        m_verticesTr.emplace_back(vertex,  0.5f);
    }

    if (m_primType == LINE_LOOP && !m_loopDone)
        close_loop();
}

void
LineRenderer::triangulate()
{
    if (m_triangulated)
    {
        if (m_primType == LINE_LOOP && !m_loopDone)
            close_loop();
        return;
    }

    if ((m_hints & PREFER_SIMPLE_TRIANGLES) != 0 && m_primType != LINES)
    {
        triangulate_vertices_as_segments();
    }
    else if (m_primType == LINE_STRIP || m_primType == LINE_LOOP)
    {
        triangulate_vertices();
    }
    else
    {
        triangulate_segments();
    }

    m_triangulated = true;
}

#if 0
LineRenderer::LineRenderer(const Renderer &renderer, float width, PrimType primType, VertexFormat format, StorageType storageType) :
#else
LineRenderer::LineRenderer(const Renderer &renderer, float width, PrimType primType, StorageType storageType) :
#endif
    m_renderer(renderer),
    m_width(width),
    m_primType(primType),
#if 0
    m_format(format),
#endif
    m_storageType(storageType)
{
    if (storageType == DYNAMIC_STORAGE)
        m_useTriangles = m_renderer.shouldDrawLineAsTriangles(m_width);
}

void
LineRenderer::clear()
{
    m_verticesTr.clear();
    m_segments.clear();
    m_vertices.clear();
    m_triangulated = false;
    m_loopDone = false;
}

void
LineRenderer::render(const Matrices &mvp, const Color &color, int count, int offset)
{
    m_useTriangles = m_useTriangles || m_renderer.shouldDrawLineAsTriangles(m_width);

    setup_shader(*mvp.projection, *mvp.modelview);
    setup_vbo();
#ifdef GL_ES
    glVertexAttrib4fv(CelestiaGLProgram::ColorAttributeIndex, color.toVector4().data());
#else
    glVertexAttrib4Nubv(CelestiaGLProgram::ColorAttributeIndex, color.data());
#endif

    if (m_useTriangles)
    {
        triangulate();

        if ((m_hints & PREFER_SIMPLE_TRIANGLES) != 0 && m_primType != LINES)
        {
            if (m_primType == LINE_STRIP)
                count--;
            draw_triangles(count*6, offset*6);
        }
        else if (m_primType == LINES)
        {
            draw_triangles(count*3, offset*3);
        }
        else
        {
            if (m_primType == LINE_LOOP)
                count++;
            draw_triangle_strip(count*2, offset*2);
        }
    }
    else
    {
        draw_lines(count, offset);
    }
}

void LineRenderer::addVertex(const Eigen::Vector4f &vertex)
{
    if (!m_useTriangles)
    {
        m_vertices.push_back(vertex);
    }
    else
    {
        m_verticesTr.emplace_back(vertex, -0.5f);
        m_verticesTr.emplace_back(vertex,  0.5f);
    }
}

void
LineRenderer::addVertex(const Eigen::Vector3f &vertex)
{
    if (!m_useTriangles)
    {
        m_vertices.emplace_back(vertex.x(), vertex.y(), vertex.z(), 1.0f);
    }
    else
    {
        Eigen::Vector4f v(vertex.x(), vertex.y(), vertex.z(), 1.0f);
        m_verticesTr.emplace_back(v, -0.5f);
        m_verticesTr.emplace_back(v,  0.5f);
    }
}

void
LineRenderer::addVertex(float x, float y, float z, float w)
{
    if (!m_useTriangles)
    {
        m_vertices.emplace_back(x, y, z, w);
    }
    else
    {
        Eigen::Vector4f v(x, y, z, w);
        m_verticesTr.emplace_back(v, -0.5f);
        m_verticesTr.emplace_back(v,  0.5f);
    }
}

void
LineRenderer::addVertex(float x, float y, float z)
{
    if (!m_useTriangles)
    {
        m_vertices.emplace_back(x, y, z, 1.0f);
    }
    else
    {
        Eigen::Vector4f v(x, y, z, 1.0f);
        m_verticesTr.emplace_back(v, -0.5f);
        m_verticesTr.emplace_back(v,  0.5f);
    }
}

void
LineRenderer::addVertex(float x, float y)
{
    if (!m_useTriangles)
    {
        m_vertices.emplace_back(x, y, 0.0f, 1.0f);
    }
    else
    {
        Eigen::Vector4f v(x, y, 0.0f, 1.0f);
        m_verticesTr.emplace_back(v, -0.5f);
        m_verticesTr.emplace_back(v,  0.5f);
    }
}

void
LineRenderer::addSegment(const Eigen::Vector4f &vertex1, const Eigen::Vector4f &vertex2)
{
    if (!m_useTriangles)
    {
        m_vertices.push_back(vertex1);
        m_vertices.push_back(vertex2);
    }
    else
    {
        triangulate_segments();
    }
}

void
LineRenderer::addSegment(const Eigen::Vector3f &vertex1, const Eigen::Vector3f &vertex2)
{
    if (!m_useTriangles)
    {
        m_vertices.push_back(Eigen::Vector4f(vertex1.x(), vertex1.y(), vertex1.z(), 1.0f));
        m_vertices.push_back(Eigen::Vector4f(vertex2.x(), vertex2.y(), vertex2.z(), 1.0f));
    }
    else
    {
        triangulate_segments();
    }
}

void
LineRenderer::setPrimitive(LineRenderer::PrimType primType)
{
    m_primType = primType;
}

void
LineRenderer::setVertexCount(int count)
{
    m_vertices.reserve(count);
}

void
LineRenderer::setHints(int hints)
{
    m_hints = hints;
}

void
LineRenderer::setCustomShader(CelestiaGLProgram *prog)
{
    m_prog = prog;
}
} // namespace
