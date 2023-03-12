#include <celengine/glsupport.h>
#include <celengine/render.h>
#include <celrender/gl/buffer.h>
#include <celrender/gl/vertexobject.h>
#include <celutil/color.h>
#include "largestarrenderer.h"

namespace util = celestia::util;

namespace celestia::render
{

LargeStarRenderer::LargeStarRenderer(Renderer &renderer) :
    m_renderer(renderer)
{
}

LargeStarRenderer::~LargeStarRenderer() = default;

void
LargeStarRenderer::render(
    const Eigen::Vector3f &position,
    const Color           &color,
    float                  size,
    const Matrices        &mvp)
{
    auto *prog = m_renderer.getShaderManager().getShader("largestar");
    if (prog == nullptr)
        return;

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE};
    ps.depthTest = true;
    m_renderer.setPipelineState(ps);

    // Draw billboard for large points
    prog->use();
    prog->samplerParam("starTex") = 0;
    prog->setMVPMatrices(*mvp.projection, *mvp.modelview);
    prog->vec4Param("color") = color.toVector4();
    prog->vec3Param("center") = position;
    prog->floatParam("pointWidth") = size / m_renderer.getWindowWidth() * 2.0f;
    prog->floatParam("pointHeight") = size / m_renderer.getWindowHeight() * 2.0f;

    initialize();
    m_vo->draw();
}

void
LargeStarRenderer::initialize()
{
    if (m_initialized)
        return;

    m_initialized = true;

    const std::array texCoords = {
        // offset     // texCoords
        -0.5f,  0.5f, 0.0f, 0.0f,
        -0.5f, -0.5f, 0.0f, 1.0f,
         0.5f, -0.5f, 1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f, 0.0f,
         0.5f, -0.5f, 1.0f, 1.0f,
         0.5f, 0.5f,  1.0f, 0.0f,
    };

    m_bo = std::make_unique<gl::Buffer>(gl::Buffer::TargetHint::Array, texCoords);
    m_vo = std::make_unique<gl::VertexObject>();

    m_vo->setCount(6);
    m_vo->addVertexBuffer(
        *m_bo,
        CelestiaGLProgram::VertexCoordAttributeIndex,
        2,
        gl::VertexObject::DataType::Float,
        false,
        4 * sizeof(float),
        0);
    m_vo->addVertexBuffer(
        *m_bo,
        CelestiaGLProgram::TextureCoord0AttributeIndex,
        2,
        gl::VertexObject::DataType::Float,
        false,
        4 * sizeof(float),
        2 * sizeof(float));
}

}
