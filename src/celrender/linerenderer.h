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
    enum
    {
        VF_VOID             = 0,
        VF_FLOAT            = 1,
        VF_UBYTE            = 2,
        VF_FLOAT_BIT        = 0,
        VF_UBYTE_BIT        = 8,
        VF_COLOR_POS        = 4,
        VF_P3F              = VF_FLOAT_BIT | 3,
        VF_C4F              = (VF_FLOAT_BIT | 4)<< VF_COLOR_POS,
        VF_C4UB             = (VF_UBYTE_BIT | 4) << VF_COLOR_POS,
        VF_COUNT_MASK       = 7,
        VF_ATTR_MASK        = 15,
    };

public:
    enum class StorageType
    {
        Stream                        = 0x88E0, // update several times per frame
        Static                        = 0x88E4, // don't update
        Dynamic                       = 0x88E8, // update once per frame
    };

    enum class PrimType
    {
        Lines                         = 0x0001,
        LineLoop                      = 0x0002,
        LineStrip                     = 0x0003,
    };

    enum Hints
    {
        PREFER_SIMPLE_TRIANGLES       = 0x0001,
        DISABLE_FISHEYE_TRANFORMATION = 0x0002,
    };

    enum class VertexFormat
    {
        P3F                           = VF_P3F,             // Position = float[3]
        P3F_C4F                       = VF_P3F | VF_C4F,    // Position = float[3], Color = float[4]
        P3F_C4UB                      = VF_P3F | VF_C4UB,   // Position = float[3], Color = unsigned char[4]
    };

    struct Vertex
    {
        explicit Vertex(const Eigen::Vector3f &pos) :
            pos(pos)
        {
        }
        Vertex(const Eigen::Vector3f &pos, const Color &color) :
            pos(pos),
            color(color)
        {
        }
        Eigen::Vector3f pos;
        Color           color;
    };

    LineRenderer(const Renderer &renderer,
                 float width = 1.0f,
                 PrimType primType = PrimType::Lines,
                 StorageType storageType = StorageType::Dynamic,
                 VertexFormat format = VertexFormat::P3F);
    ~LineRenderer() = default;

    void clear();
    void orphan() const;
    void finish();
    void startUpdate();
    void addVertex(const Vertex &vertex);
    void addVertex(const Eigen::Vector3f &pos, const Color &color);
    void addVertex(const Eigen::Vector3f &pos);
    void addVertex(float x, float y, float z);
    void addVertex(float x, float y);
    void addSegment(const Eigen::Vector3f &pos1, const Eigen::Vector3f &pos2);
    void dropLast();
    void setVertexCount(int count);
    void setHints(int hints);
    void setCustomShader(CelestiaGLProgram *prog);
    float getWidth() const { return m_width; }
    void prerender();
    void render(const Matrices &mvp, int count, int offset = 0);
    void render(const Matrices &mvp, const Color &color, int count, int offset = 0);

private:
    // LineVertex is used to draw lines with triangles with GL_TRIANGLE_STRIP
    struct LineVertex
    {
        LineVertex(const Vertex &point, float scale) :
            point(point),
            scale(scale)
        {};
        Vertex point;
        float  scale;
    };

    // LineSegment is used to draw lines with triangles with GL_TRIANGLES
    struct LineSegment
    {
        LineSegment(const Vertex &point1, const Vertex &point2, float scale) :
            point1(point1),
            point2(point2),
            scale(scale)
        {};
        Vertex point1;
        Vertex point2;
        float  scale;
    };

    void draw_lines(int count, int offset) const;
    void draw_triangles(int count, int offset) const;
    void draw_triangle_strip(int count, int offset) const;
    void setup_shader();
    void setup_vbo();
    void create_vbo_lines();
    void setup_vbo_lines();
    void create_vbo_triangles();
    void setup_vbo_triangles();
    void triangulate();
    void triangulate_segments();
    void triangulate_vertices_as_segments();
    void triangulate_vertices();
    void close_loop();
    void close_strip();
    void add_segment_points(const Vertex &point1, const Vertex &point2);
    int pos_count() const;
    int color_count() const;
    int color_type() const;

    std::vector<LineVertex>         m_verticesTr;
    std::vector<LineSegment>        m_segments;
    std::vector<Vertex>             m_vertices;
    std::vector<Color>              m_colors;
    std::unique_ptr<celgl::VertexObject> m_lnVertexObj;
    std::unique_ptr<celgl::VertexObject> m_trVertexObj;
    const Renderer                 &m_renderer;
    float                           m_width;
    PrimType                        m_primType;
    StorageType                     m_storageType;
    VertexFormat                    m_format;
    int                             m_hints             { 0     };
    bool                            m_useTriangles      { false };
    bool                            m_triangulated      { false };
    bool                            m_loopDone          { false };
    bool                            m_inUse             { false };
    CelestiaGLProgram              *m_prog              { nullptr };
};
}
