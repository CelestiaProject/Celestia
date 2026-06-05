// largestarrenderer.h

#pragma once

#include <array>
#include <memory>
#include <vector>

#include <Eigen/Core>

#include <celengine/starpipelineowner.h>

class Color;
class Renderer;
class Texture;
class CelestiaGLProgram;

namespace celestia::gl
{
class Buffer;
class VertexObject;
} // namespace celestia::gl

namespace celestia::render
{

// Batched textured-billboard renderer for point-sprite stars whose
// gl_PointSize would exceed the driver's GL_ALIASED_POINT_SIZE_RANGE.
// Each star expands to a 6-vertex quad and the whole frame's oversized
// stars draw in a single call.
class LargeStarRenderer : public StarPipelineFlushable
{
public:
    using capacity_t = unsigned int;

    explicit LargeStarRenderer(Renderer &renderer, capacity_t capacity = 512);
    ~LargeStarRenderer() override;

    LargeStarRenderer() = delete;
    LargeStarRenderer(const LargeStarRenderer&) = delete;
    LargeStarRenderer(LargeStarRenderer&&) = delete;
    LargeStarRenderer& operator=(const LargeStarRenderer&) = delete;
    LargeStarRenderer& operator=(LargeStarRenderer&&) = delete;

    void start();
    void render();
    void finish() override;

    void addStar(const Eigen::Vector3f &center, const Color &color, float size);

    void setTexture(Texture *texture) { m_texture = texture; }

private:
    struct StarVertex
    {
        Eigen::Vector3f              center;
        float                        size;
        std::array<unsigned char, 4> color;
        std::array<signed char, 2>   corner;  // ±64  -> ±0.5  (signed byte normalized)
        std::array<unsigned char, 2> uv;      // 0/255 -> 0/1  (unsigned byte normalized)
    };

    void makeCurrent();
    void setupVertexArrayObject();

    Renderer                       &m_renderer;
    capacity_t                      m_capacity;
    capacity_t                      m_nStars        { 0 };
    std::vector<StarVertex>         m_vertices;

    Texture                        *m_texture       { nullptr };

    CelestiaGLProgram              *m_prog          { nullptr };
    std::unique_ptr<gl::Buffer>     m_bo;
    std::unique_ptr<gl::VertexObject> m_vo;
    bool                            m_initialized   { false };
};

} // namespace celestia::render
