// largestarrenderer.cpp

#include "largestarrenderer.h"

#include <array>
#include <cstddef>

#include <celengine/glsupport.h>
#include <celengine/render.h>
#include <celengine/shadermanager.h>
#include <celengine/texture.h>
#include <celrender/gl/buffer.h>
#include <celrender/gl/vertexobject.h>
#include <celutil/array_view.h>
#include <celutil/color.h>

namespace gl  = celestia::gl;
namespace util = celestia::util;

namespace celestia::render
{

namespace
{
struct Corner
{
    signed char   x, y;
    unsigned char u, v;
};

constexpr std::array<Corner, 6> kQuadCorners = {{
    { -64,  64, 0, 0   },
    { -64, -64, 0, 255 },
    {  64, -64, 255, 255 },
    { -64,  64, 0, 0   },
    {  64, -64, 255, 255 },
    {  64,  64, 255, 0 },
}};
} // namespace

LargeStarRenderer::LargeStarRenderer(Renderer &renderer, capacity_t capacity) :
    m_renderer(renderer),
    m_capacity(capacity),
    m_vertices(static_cast<std::size_t>(capacity) * 6)
{
}

LargeStarRenderer::~LargeStarRenderer() = default;

void
LargeStarRenderer::start()
{
    m_prog = m_renderer.getShaderManager().getShader(StaticShader::LargeStar);
    m_nStars = 0;
}

void
LargeStarRenderer::render()
{
    if (m_nStars == 0 || m_prog == nullptr)
        return;

    makeCurrent();

    m_bo->invalidateData().setData(
        util::array_view(m_vertices.data(), static_cast<std::size_t>(m_nStars) * 6),
        gl::Buffer::BufferUsage::StreamDraw);

    m_vo->draw(static_cast<int>(m_nStars) * 6);
    m_nStars = 0;
}

void
LargeStarRenderer::finish()
{
    render();
    m_renderer.starPipelineOwner().clearIfActive(this);
}

void
LargeStarRenderer::makeCurrent()
{
    auto &owner = m_renderer.starPipelineOwner();
    if (owner.isActive(this) || m_prog == nullptr)
        return;

    owner.setActive(this);

    // Match the pipeline state callers used to set per-draw.
    Renderer::PipelineState ps;
    ps.blending  = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE};
    ps.depthTest = true;
    m_renderer.setPipelineState(ps);

    if (m_texture != nullptr)
        m_texture->bind();

    setupVertexArrayObject();

    m_prog->use();
    m_prog->samplerParam("starTex") = 0;
    m_prog->setMVPMatrices(m_renderer.getCurrentProjectionMatrix(),
                           m_renderer.getCurrentModelViewMatrix());

    std::array<int, 4> vp{};
    m_renderer.getViewport(vp);
    float invW = (vp[2] > 0) ? (1.0f / static_cast<float>(vp[2])) : 0.0f;
    float invH = (vp[3] > 0) ? (1.0f / static_cast<float>(vp[3])) : 0.0f;
    m_prog->vec2Param("viewportRcp") = Eigen::Vector2f(invW, invH);
}

void
LargeStarRenderer::setupVertexArrayObject()
{
    if (m_initialized)
        return;

    m_initialized = true;

    m_bo = std::make_unique<gl::Buffer>(gl::Buffer::TargetHint::Array);
    m_vo = std::make_unique<gl::VertexObject>(gl::VertexObject::Primitive::Triangles);

    m_vo->addVertexBuffer(
        *m_bo,
        CelestiaGLProgram::VertexCoordAttributeIndex,
        2,
        gl::VertexObject::DataType::Byte,
        true,
        sizeof(StarVertex),
        offsetof(StarVertex, corner));

    m_vo->addVertexBuffer(
        *m_bo,
        CelestiaGLProgram::NormalAttributeIndex,
        3,
        gl::VertexObject::DataType::Float,
        false,
        sizeof(StarVertex),
        offsetof(StarVertex, center));

    m_vo->addVertexBuffer(
        *m_bo,
        CelestiaGLProgram::TextureCoord0AttributeIndex,
        2,
        gl::VertexObject::DataType::UnsignedByte,
        true,
        sizeof(StarVertex),
        offsetof(StarVertex, uv));

    m_vo->addVertexBuffer(
        *m_bo,
        CelestiaGLProgram::ColorAttributeIndex,
        4,
        gl::VertexObject::DataType::UnsignedByte,
        true,
        sizeof(StarVertex),
        offsetof(StarVertex, color));

    m_vo->addVertexBuffer(
        *m_bo,
        CelestiaGLProgram::IntensityAttributeIndex,
        1,
        gl::VertexObject::DataType::Float,
        false,
        sizeof(StarVertex),
        offsetof(StarVertex, size));
}

void
LargeStarRenderer::addStar(const Eigen::Vector3f &center,
                           const Color           &color,
                           float                  size)
{
    if (m_nStars < m_capacity)
    {
        std::array<unsigned char, 4> packedColor{};
        color.get(packedColor.data());

        StarVertex *out = &m_vertices[static_cast<std::size_t>(m_nStars) * 6];
        for (std::size_t i = 0; i < 6; ++i)
        {
            out[i].center = center;
            out[i].size   = size;
            out[i].color  = packedColor;
            out[i].corner = { kQuadCorners[i].x, kQuadCorners[i].y };
            out[i].uv     = { kQuadCorners[i].u, kQuadCorners[i].v };
        }
        m_nStars++;
    }

    if (m_nStars == m_capacity)
    {
        render();
        m_nStars = 0;
    }
}

} // namespace celestia::render
