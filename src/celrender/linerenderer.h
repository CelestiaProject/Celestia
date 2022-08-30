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

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <Eigen/Core>
#include <celengine/vertexobject.h>
#include <celutil/color.h>

class CelestiaGLProgram;
class Renderer;
struct Matrices;

namespace celestia::engine
{
class LineRenderer
{
public:
    enum StorageType
    {
        STREAM_STORAGE                = 0x88E0,
        STATIC_STORAGE                = 0x88E4,
        DYNAMIC_STORAGE               = 0x88E8,
    };

    enum PrimType
    {
        LINES                         = 0x0001,
        LINE_LOOP                     = 0x0002,
        LINE_STRIP                    = 0x0003,
    };

    enum Hints
    {
        PREFER_SIMPLE_TRIANGLES       = 0x0001,
        DISABLE_FISHEYE_TRANFORMATION = 0x0002,
    };

    enum VertexFormat
    {
        POSITION2F                    = 0x0002,
        POSITION3F                    = 0x0003,
        POSITION4F                    = 0x0004,
        POSITION3F_COLOR4F            = 0x0043,
        POSITION3F_COLOR4UB           = 0x00C3, //Position3fColor4ub
    };

#if 0
    enum VertexFormat
    {
        VERTEX_POSITION_2F,
        VERTEX_POSITION_3F,
        VERTEX_POSITION_4F,
    };

    LineRenderer(const Renderer &renderer, float width = 1.0f, PrimType primType = LINES, VertexFormat format = VERTEX_POSITION_3F, StorageType storageType = DYNAMIC_STORAGE);
#endif
    LineRenderer(const Renderer &renderer, float width = 1.0f, PrimType primType = LINES, StorageType storageType = DYNAMIC_STORAGE);
    ~LineRenderer() = default;

    void clear();
    void addVertex(const Eigen::Vector4f &vertex);
    void addVertex(const Eigen::Vector3f &vertex);
    void addVertex(float x, float y, float z, float w);
    void addVertex(float x, float y, float z);
    void addVertex(float x, float y);
    void addSegment(const Eigen::Vector4f &vertex1, const Eigen::Vector4f &vertex2);
    void addSegment(const Eigen::Vector3f &vertex1, const Eigen::Vector3f &vertex2);
    void setVertexCount(int count);
    void setHints(int hints);
    void setCustomShader(CelestiaGLProgram *prog);
    [[ deprecated ]] void setPrimitive(PrimType);

    float getWidth() const { return m_width; }

    void render(const Matrices &mvp, const Color &color, int count, int offset = 0);

private:
    // LineVertex is used to draw lines with triangles with GL_TRIANGLE_STRIP
    struct LineVertex
    {
        LineVertex(const Eigen::Vector4f &point, float scale) :
            point(point),
            scale(scale)
        {};
        Eigen::Vector4f point;
        float           scale;
    };

    // LineSegment is used to draw lines with triangles with GL_TRIANGLES
    struct LineSegment
    {
        LineSegment(const Eigen::Vector4f &point1, const Eigen::Vector4f &point2, float scale) :
            point1(point1),
            point2(point2),
            scale(scale)
        {};
        Eigen::Vector4f point1;
        Eigen::Vector4f point2;
        float           scale;
    };

    void draw_lines(int count, int offset);
    void draw_triangles(int count, int offset);
    void draw_triangle_strip(int count, int offset);
    void setup_shader(const Eigen::Matrix4f &projection, const Eigen::Matrix4f &modelview);
    void setup_vbo();
    void triangulate();
    void triangulate_segments();
    void triangulate_vertices_as_segments();
    void triangulate_vertices();
    void close_loop();
    void add_segment_points(const Eigen::Vector4f &point1, const Eigen::Vector4f &point2);

    std::vector<LineVertex>         m_verticesTr;
    std::vector<LineSegment>        m_segments;
    std::vector<Eigen::Vector4f>    m_vertices;
    std::unique_ptr<celgl::VertexObject> m_lnVertexObj;
    std::unique_ptr<celgl::VertexObject> m_trVertexObj;
    const Renderer                 &m_renderer;
    float                           m_width;
    PrimType                        m_primType;
#if 0
    VertexFormat                    m_format;
#endif
    StorageType                     m_storageType;
    int                             m_hints             { 0     };
    bool                            m_useTriangles      { false };
    bool                            m_triangulated      { false };
    bool                            m_loopDone          { false };
    CelestiaGLProgram              *m_prog              { nullptr };
};
}
