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
#include <celrender/vertexobject.h>
#include <celutil/color.h>

class CelestiaGLProgram;
class Renderer;
struct Matrices;

namespace celestia::render
{
/**
 * \class LineRenderer linerenderer.h celrender/linerenderer.h
 *
 * @brief Render lines converting them into triangles if required.
 *
 * Render lines converting them into triangles if required. Conversation into triangles is performed
 * only if requested line is wider that maximal line width supported by the GL implementation. In
 * the most cases desktop OpenGL drivers support line wider than 1px, while OpenGL ES mobile drivers
 * support only 1px wide lines.
 * For lines which are not updated (static storage) conversation into triangles is performed before
 * the actual rendering is done. For lines with dynamic or stream storage conversation into
 * triangles is performed immediatelly when a new vertex or segment is added.
 *
 * Worflow:
 *   1. create lr
 *   2. lr.addVertex()/lr.addSegment()
 *   3. <em>(optionaly)</em> lr.prerender()
 *   4. lr.render()
 *   5. <em>(optionaly with static storage)</em> lr.clear()
 *   6. lr.finish()
 */
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
    //! Defines the GPU storage type, i.e. vertices update mode.
    enum class StorageType
    {
        //! Update multiple times per frame
        Stream                        = 0x88E0,
        //! Don't update
        Static                        = 0x88E4,
        //! Update once per frame
        Dynamic                       = 0x88E8,
    };

    //! Defines the primitive used to render lines.
    enum class PrimType
    {
        //! Line segments, i.e. GL_LINES
        Lines                         = 0x0001,
        //! Line loop, i.e. GL_LINE_LOOP
        LineLoop                      = 0x0002,
        //! Line strip, i.e. GL_LINE_STRIP
        LineStrip                     = 0x0003,
    };

    //! Bit flags to defines additional hints to renderer.
    enum Hints
    {
        //! Convert LineStrip and LineLoop into Lines before triangulation.
        PREFER_SIMPLE_TRIANGLES       = 0x0001,
        //! If default rendering mode enables fish eye transformation, then disable it.
        DISABLE_FISHEYE_TRANFORMATION = 0x0002,
    };

    /**
     * @brief Define vertex format (layout).
     *
     * Define vertex format (layout), e.g. number and type of elements in position and color
     * attributes.
     */
    enum class VertexFormat
    {
        //! Position = float[3]
        P3F                           = VF_P3F,
        //! Position = float[3], color = float[4]
        P3F_C4F                       = VF_P3F | VF_C4F,
        //! Position = float[3], color = unsigned char[4]
        P3F_C4UB                      = VF_P3F | VF_C4UB,
    };

    //! Defines a vertex (vertices positions and colors).
    struct Vertex
    {
        explicit Vertex(const Eigen::Vector3f &pos) :
            pos(pos)
        {}
        Vertex(const Eigen::Vector3f &pos, const Color &color) :
            pos(pos),
            color(color)
        {}
        Eigen::Vector3f pos;
        Color           color;
    };

    /**
     * @brief Construct a new LineRenderer object.
     *
     * @param renderer Reference to Renderer object.
     * @param width Line width in pixels.
     * @param primType Line pritive type (Lines, LineStrip, LineLoop).
     * @param storageType Storage type (how the vertices are updated).
     * @param format Vertex format.
     */
    LineRenderer(const Renderer &renderer,
                 float width = 1.0f,
                 PrimType primType = PrimType::Lines,
                 StorageType storageType = StorageType::Dynamic,
                 VertexFormat format = VertexFormat::P3F);

    ~LineRenderer() = default;

    //! Clear CPU side memory buffers
    void clear();

    //! Clear GPU side memory buffers
    void orphan() const;

    //! Finish renderring
    void finish();

    //! Enable triangulation mode for dynamic lines if required
    void startUpdate();

    //! Add a new vertex with position and color
    void addVertex(const Vertex &vertex);

    //! Add a new vertex with position and color
    void addVertex(const Eigen::Vector3f &pos, const Color &color);

    //! Add a new vertex with position only
    void addVertex(const Eigen::Vector3f &pos);

    //! Add a new vertex with position only
    void addVertex(float x, float y, float z = 0.0f);

    //! Add a new line segment, use with PrimType=Lines
    void addSegment(const Eigen::Vector3f &pos1, const Eigen::Vector3f &pos2);

    //! Remove last vertex, do nothing when PrimType=Lines
    void dropLast();

    /**
     * @brief Preallocate CPU buffer.
     *
     * @param count Number of expected vertices.
     */
    void setVertexCount(int count);

    /**
     * @brief Set rendering hints
     *
     * @param hints OR-ed LineRenderer::Hints
     */
    void setHints(int hints);

    //! Set a custom shader compiled into CelestiaGLProgram to use instead of default one
    void setCustomShader(CelestiaGLProgram *prog);

    /**
     * @brief Return the line width
     *
     * @return Current line width
     */
    float getWidth() const { return m_width; }

    //! Triangulate lines if required, transfer data to GPU, enable shaders, etc
    void prerender();

    /**
     * @brief Render lines with a color defined by vertex color attribute.
     *
     * @param mvp Specifies the Projection and ModelView matrices.
     * @param count Specifies the number of primitives to draw.
     * @param offset Specifies the starting primitive to draw.
     */
    void render(const Matrices &mvp, int count, int offset = 0);

    /**
     * @brief Render lines with a color provided.
     *
     * @param mvp Specifies the Projection and ModelView matrices.
     * @param color Specifies the line color.
     * @param count Specifies the number of primitives to draw.
     * @param offset Specifies the starting primitive to draw.
     */
    void render(const Matrices &mvp, const Color &color, int count, int offset = 0);

private:
    //! LineVertex is used to draw lines with triangles with GL_TRIANGLE_STRIP
    struct LineVertex
    {
        LineVertex(const Vertex &point, float scale) :
            point(point),
            scale(scale)
        {}
        Vertex point;
        float  scale;
    };

    //! LineSegment is used to draw lines with triangles with GL_TRIANGLES
    struct LineSegment
    {
        LineSegment(const Vertex &point1, const Vertex &point2, float scale) :
            point1(point1),
            point2(point2),
            scale(scale)
        {}
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
    std::unique_ptr<VertexObject>   m_lnVertexObj;
    std::unique_ptr<VertexObject>   m_trVertexObj;
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
