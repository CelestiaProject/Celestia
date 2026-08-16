// galaxyrenderer.cpp
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel, Fridger Schrempp, and Toti
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "galaxyrenderer.h"

#include <cassert>
#include <cstdint>
#include <cmath>
#include <memory>

#include <celengine/galaxy.h>
#include <celengine/galaxyform.h>
#include <celengine/glsupport.h>
#include <celengine/render.h>
#include <celengine/shadermanager.h>
#include <celengine/texture.h>
#include <celmath/geomutil.h>
#include <celrender/gl/buffer.h>
#include <celrender/gl/vertexobject.h>

using celestia::engine::GalacticFormManager;

namespace celestia::render
{
namespace
{
constexpr int kGalaxyTextureSize = 128;
constexpr float kSpriteScaleFactor = 1.0f / 1.55f;

void
galaxyTextureEval(float u, float v, std::uint8_t *pixel)
{
    float r = std::max(0.0f, 0.9f - std::hypot(u, v));
    *pixel = static_cast<std::uint8_t>(r * 255.99f);
}

void
colorTextureEval(float u, float /*v*/, std::uint8_t *pixel)
{
    auto i = static_cast<int>((u * 0.5f + 0.5f) * 255.99f); // [-1, 1] -> [0, 255]

    // generic Hue profile as deduced from true-color imaging for spirals
    // Hue in degrees
    float hue = (i < 28)
        ? 25.0f * std::tanh(0.0615f * static_cast<float>(27 - i))
        : 245.0f;
    Color::fromHSV(hue, 0.20f, 1.0f).get(pixel);
}

} // anonymous namespace

struct GalaxyRenderer::RenderData
{
    explicit RenderData(gl::VertexObject &&vo) :
        vo(std::move(vo))
    {
    }

    gl::VertexObject vo;
};

struct GalaxyRenderer::Object
{
    Object(const Eigen::Vector3f &offset, float brightness, float nearZ, float farZ, const Galaxy *galaxy) :
        offset(offset),
        brightness(brightness),
        nearZ(nearZ),
        farZ(farZ),
        galaxy(galaxy)
    {
    }

    Eigen::Vector3f offset; // distance to the galaxy
    float           brightness;
    float           nearZ;  // if nearZ != & farZ != then use custom projection matrix
    float           farZ;
    const Galaxy   *galaxy;
};

GalaxyRenderer::GalaxyRenderer(Renderer &renderer) :
    m_renderer(renderer)
{
    m_objects.reserve(1024);
}

GalaxyRenderer::~GalaxyRenderer() = default; // define here as Object is not defined in the header file

void
GalaxyRenderer::update(const Eigen::Quaternionf &viewerOrientation, float pixelSize, float fov, float zoom)
{
    m_viewerOrientation = viewerOrientation;
    m_viewMat = viewerOrientation.conjugate().toRotationMatrix();
    m_pixelSize = pixelSize;
    m_fov = fov;
    m_zoom = zoom;
}

void
GalaxyRenderer::add(const Galaxy *galaxy, const Eigen::Vector3f &offset, float brightness, float nearZ, float farZ)
{
    m_objects.emplace_back(offset, brightness, nearZ, farZ, galaxy);
}

void
GalaxyRenderer::render()
{
    if (m_objects.empty())
        return;

    CelestiaGLProgram *prog =  m_renderer.getShaderManager().getShader(StaticShader::Galaxy);
    if (prog == nullptr)
    {
        m_objects.clear();
        return;
    }

    initializeGL(prog);

    bindTextures();

    prog->use();
    prog->samplerParam("galaxyTex") = 0;
    prog->samplerParam("colorTex") = 1;
    prog->mat3Param("viewMat") = m_viewMat;

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    ps.smoothLines = true;
    m_renderer.setPipelineState(ps);

    for (const auto &obj : m_objects)
    {
        float brightness = 0.0f;
        float size = 0.0f;
        float minimumFeatureSize = 0.0f;
        Eigen::Matrix4f m;
        Eigen::Matrix4f pr;
        int nPoints = 0;

        if (!getRenderInfo(obj, brightness, size, minimumFeatureSize, m, pr, nPoints))
            continue;

        prog->setMVPMatrices(pr, m_renderer.getModelViewMatrix());

        prog->floatParam("size")               = size;
        prog->floatParam("brightness")         = brightness;
        prog->mat4Param("m")                   = m;

        m_renderData[obj.galaxy->getFormId()].vo.drawInstances(nPoints);
    }

    glActiveTexture(GL_TEXTURE0);
    m_objects.clear();
}

bool
GalaxyRenderer::getRenderInfo(const GalaxyRenderer::Object &obj, float &brightness, float &size, float &minimumFeatureSize, Eigen::Matrix4f &m, Eigen::Matrix4f &pr, int &nPoints) const
{
    const auto* galacticForm = GalacticFormManager::get()->getForm(obj.galaxy->getFormId());
    if (galacticForm == nullptr)
        return false;

    // We'll first see if the galaxy's apparent size is big enough to
    // be noticeable on screen; if it's not we'll break right here,
    // avoiding all the overhead of the matrix transformations and
    // GL state changes:
    float distanceToDSO = std::max(0.0f, obj.offset.norm() - obj.galaxy->getRadius());
    minimumFeatureSize = m_pixelSize * distanceToDSO;
    size = 2.0f * obj.galaxy->getRadius();
    if (size < minimumFeatureSize)
        return false;

    m = (
        Eigen::Translation3f(obj.offset) *
        Eigen::Affine3f(obj.galaxy->getOrientation().conjugate()) *
        Eigen::Scaling(galacticForm->scale * size)
    ).matrix();

    brightness = obj.galaxy->getBrightnessCorrection(obj.offset) * obj.brightness;

    if (obj.nearZ != 0.0f && obj.farZ != 0.0f)
        m_renderer.buildProjectionMatrix(pr, obj.nearZ, obj.farZ, m_zoom);
    else
        pr = m_renderer.getProjectionMatrix();

    const auto &points = galacticForm->blobs;
    auto pointCount = static_cast<int>(static_cast<float>(points.size()) * std::clamp(obj.galaxy->getDetail(), 0.0f, 1.0f));
    // find proper nPoints count
    if (minimumFeatureSize > 0.0f)
    {
        auto power = static_cast<unsigned>(std::log(minimumFeatureSize/size)/std::log(kSpriteScaleFactor));
        if (power < std::numeric_limits<decltype(pointCount)>::digits)
            pointCount = std::min(pointCount, 1 << power);
    }
    nPoints = pointCount;
    return true;
}

void
GalaxyRenderer::bindTextures()
{
    if (m_galaxyTex == nullptr)
    {
        m_galaxyTex = ImageTexture::createProcedural(kGalaxyTextureSize,
                                                     kGalaxyTextureSize,
                                                     engine::PixelFormat::Luminance,
                                                     &galaxyTextureEval);
    }

    assert(m_galaxyTex != nullptr);
    glActiveTexture(GL_TEXTURE0);
    m_galaxyTex->bind();

    if (m_colorTex == nullptr)
    {
        // Color values are authored in sRGB; mark the texture accordingly so
        // GL linearizes them when the shader samples the colour lookup table.
        m_colorTex = ImageTexture::createProcedural(256, 1, engine::PixelFormat::sRGBA,
                                                    &colorTextureEval,
                                                    Texture::EdgeClamp,
                                                    Texture::NoMipMaps);
    }

    assert(m_colorTex != nullptr);
    glActiveTexture(GL_TEXTURE1);
    m_colorTex->bind();
}

void
GalaxyRenderer::initializeGL(const CelestiaGLProgram *prog)
{
    struct GalaxyInstance
    {
        Eigen::Matrix<GLshort, 3, 1>  position;
        GLushort size;       // we scale blob by size=kSpriteScaleFactor**n
        GLubyte  colorIndex; // color index [0; 255]
        GLubyte  brightness; // blob brightness [0.0; 1.0] packed as normalized byte
    };

    if (m_initialized)
        return;

    m_initialized = true;

    constexpr std::array<GLubyte, 8> texCoords
    {
        0, 0, 255, 0, 0, 255, 255, 255,
    };

    auto texCoordBuffer = gl::Buffer::create(gl::Buffer::TargetHint::Array, texCoords);

    auto sizeLoc = prog->attribIndex("in_Size");
    auto colorLoc = prog->attribIndex("in_ColorIndex");
    auto brightnessLoc = prog->attribIndex("in_Brightness");

    const auto *gm = GalacticFormManager::get();
    std::vector<GalaxyInstance> glVertices;

    for (int count = gm->getCount(), id = 0; id < count; ++id)
    {
        const auto* form = gm->getForm(id);
        if (!form)
        {
            m_renderData.emplace_back(gl::VertexObject{});
            continue;
        }

        const auto &points = form->blobs;
        glVertices.reserve(points.size());

        float sizeFactor = std::numeric_limits<GLushort>::max();
        for (unsigned int i = 0, pow2 = 1; i < points.size(); ++i)
        {
            if ((i & pow2) != 0)
            {
                pow2 <<= 1;
                sizeFactor *= kSpriteScaleFactor;
            }

            GalaxyInstance& gi = glVertices.emplace_back();
            Eigen::Vector3f p = points[i].position * std::numeric_limits<GLshort>::max();
            gi.position   = p.cast<GLshort>();
            gi.size       = static_cast<GLushort>(sizeFactor);
            gi.colorIndex = points[i].colorIndex;
            gi.brightness = points[i].brightness;
        }

        auto bo = gl::Buffer::create(gl::Buffer::TargetHint::Array, glVertices);

        gl::VertexObject vo(gl::VertexObject::Primitive::TriangleStrip);
        vo.setCount(4);

        vo.addInstanceBuffer(bo, CelestiaGLProgram::VertexCoordAttributeIndex,
                             3, gl::VertexObject::DataType::Short,
                             true, sizeof(GalaxyInstance), offsetof(GalaxyInstance, position));
        vo.addInstanceBuffer(bo, sizeLoc, 1, gl::VertexObject::DataType::UnsignedShort,
                             true, sizeof(GalaxyInstance), offsetof(GalaxyInstance, size));
        vo.addInstanceBuffer(bo, colorLoc, 1, gl::VertexObject::DataType::UnsignedByte,
                             true, sizeof(GalaxyInstance), offsetof(GalaxyInstance, colorIndex));
        vo.addInstanceBuffer(bo, brightnessLoc, 1, gl::VertexObject::DataType::UnsignedByte,
                             true, sizeof(GalaxyInstance), offsetof(GalaxyInstance, brightness));
        vo.addVertexBuffer(texCoordBuffer, CelestiaGLProgram::TextureCoord0AttributeIndex,
                           2, gl::VertexObject::DataType::UnsignedByte,
                           true);

        m_renderData.emplace_back(std::move(vo));
        glVertices.clear();
    }
}

} // namespace celestia::render
