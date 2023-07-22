// galaxyrenderer.cpp
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel, Fridger Schrempp, and Toti
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include <cstdint>
#include <cmath>

#include <celengine/galaxy.h>
#include <celengine/galaxyform.h>
#include <celengine/glsupport.h>
#include <celengine/render.h>
#include <celengine/shadermanager.h>
#include <celengine/texture.h>
#include <celmath/geomutil.h>
#include <celrender/gl/buffer.h>
#include <celrender/gl/vertexobject.h>
#include "galaxyrenderer.h"

using celestia::engine::GalacticFormManager;
using celestia::engine::GalacticForm;

namespace celestia::render
{
namespace
{
constexpr int kGalaxyTextureSize = 128;
constexpr float kSpriteScaleFactor = 1.0f / 1.55f;

void
galaxyTextureEval(float u, float v, float /*w*/, std::uint8_t *pixel)
{
    float r = std::max(0.0f, 0.9f - std::hypot(u, v));
    *pixel = static_cast<std::uint8_t>(r * 255.99f);
}

void
colorTextureEval(float u, float /*v*/, float /*w*/, std::uint8_t *pixel)
{
    auto i = static_cast<int>((u * 0.5f + 0.5f) * 255.99f); // [-1, 1] -> [0, 255]

    // generic Hue profile as deduced from true-color imaging for spirals
    // Hue in degrees
    float hue = (i < 28)
        ? 25.0f * std::tanh(0.0615f * static_cast<float>(27 - i))
        : 245.0f;
    Color::fromHSV(hue, 0.20f, 1.0f).get(pixel);
}

void
BindTextures()
{
    static Texture* galaxyTex = nullptr;
    static Texture* colorTex  = nullptr;

    if (galaxyTex == nullptr)
    {
        galaxyTex = CreateProceduralTexture(kGalaxyTextureSize,
                                            kGalaxyTextureSize,
                                            celestia::PixelFormat::LUMINANCE,
                                            galaxyTextureEval).release();
    }
    assert(galaxyTex != nullptr);
    glActiveTexture(GL_TEXTURE0);
    galaxyTex->bind();

    if (colorTex == nullptr)
    {
        colorTex = CreateProceduralTexture(256, 1, celestia::PixelFormat::RGBA,
                                           colorTextureEval,
                                           Texture::EdgeClamp,
                                           Texture::NoMipMaps).release();
    }
    assert(colorTex != nullptr);
    glActiveTexture(GL_TEXTURE1);
    colorTex->bind();
}

} // anonymous namespace

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

    if (gl::hasGeomShader())
        renderGL3();
    else
        renderGL2();

    m_objects.clear();
}

bool
GalaxyRenderer::getRenderInfo(const GalaxyRenderer::Object &obj, float &brightness, float &size, float minimumFeatureSize, Eigen::Matrix4f &m, Eigen::Matrix4f &pr, int &nPoints) const
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

struct GalaxyRenderer::RenderDataGL2
{
    RenderDataGL2(gl::Buffer &&bo, gl::Buffer &&io, gl::VertexObject &&vo) :
        bo(std::move(bo)),
        io(std::move(io)),
        vo(std::move(vo))
    {
    }
    gl::Buffer       bo{ util::NoCreateT{} };
    gl::Buffer       io{ util::NoCreateT{} };
    gl::VertexObject vo{ util::NoCreateT{} };
};

void
GalaxyRenderer::renderGL2()
{
    CelestiaGLProgram *prog =  m_renderer.getShaderManager().getShader("galaxy");
    if (prog == nullptr)
        return;

    initializeGL2(prog);

    BindTextures();

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

        m_renderDataGL2[obj.galaxy->getFormId()].vo.draw(nPoints * 6);
    }

    glActiveTexture(GL_TEXTURE0);
}

void
GalaxyRenderer::initializeGL2(const CelestiaGLProgram *prog)
{
    struct GalaxyVtx
    {
        Eigen::Matrix<GLshort, 3, 1>  position;
        GLushort size;       // we scale blob by size=kSpriteScaleFactor**n
        GLubyte  colorIndex; // color index [0; 255]
        GLubyte  brightness; // blob brightness [0.0; 1.0] packed as normalized byte
        Eigen::Matrix<GLubyte, 2, 1> texCoord;
    };

    if (m_initialized)
        return;

    m_initialized = true;

    // When geometry shader is available, we draw points and emit triangle
    // strips, otherwise, we draw triangles, so we need to buffer 4 times
    // (indexed) on all the vertices and add the texture coordinate to each
    const std::array<Eigen::Matrix<GLubyte, 2, 1>, 4> texCoords =
    {
        Eigen::Matrix<GLubyte, 2, 1>(  std::uint8_t(0),   std::uint8_t(0)),
        Eigen::Matrix<GLubyte, 2, 1>(std::uint8_t(255),   std::uint8_t(0)),
        Eigen::Matrix<GLubyte, 2, 1>(std::uint8_t(255), std::uint8_t(255)),
        Eigen::Matrix<GLubyte, 2, 1>(  std::uint8_t(0), std::uint8_t(255)),
    };

    auto sizeLoc = prog->attribIndex("in_Size");
    auto colorLoc = prog->attribIndex("in_ColorIndex");
    auto brightnessLoc = prog->attribIndex("in_Brightness");

    const auto *gm = GalacticFormManager::get();
    std::vector<GalaxyVtx> glVertices;
    std::vector<GLuint> indices;

    for (int count = gm->getCount(), id = 0; id < count; id++)
    {
        if (const auto* form = gm->getForm(id); form != nullptr)
        {
            const auto &points = form->blobs;
            glVertices.reserve(points.size() * texCoords.size());
            indices.reserve(points.size() * 6);

            float sizeFactor = std::numeric_limits<GLushort>::max();
            for (unsigned int i = 0, pow2 = 1; i < points.size(); ++i)
            {
                if ((i & pow2) != 0)
                {
                    pow2 <<= 1;
                    sizeFactor *= kSpriteScaleFactor;
                }

                GalaxyVtx v;
                Eigen::Vector3f p = points[i].position * std::numeric_limits<GLshort>::max();
                v.position   = p.cast<GLshort>();
                v.size       = static_cast<GLushort>(sizeFactor);
                v.colorIndex = points[i].colorIndex;
                v.brightness = points[i].brightness;

                for (std::size_t j = 0; j < texCoords.size(); ++j)
                {
                    v.texCoord   = texCoords[j];
                    glVertices.push_back(v);
                }

                unsigned int baseIndex = i * texCoords.size();
                indices.push_back(baseIndex);
                indices.push_back(baseIndex + 1);
                indices.push_back(baseIndex + 2);
                indices.push_back(baseIndex);
                indices.push_back(baseIndex + 2);
                indices.push_back(baseIndex + 3);
            }

            gl::Buffer bo(gl::Buffer::TargetHint::Array, glVertices);

            gl::VertexObject vo(gl::VertexObject::Primitive::Triangles);

            vo.addVertexBuffer(
                bo, CelestiaGLProgram::VertexCoordAttributeIndex,
                3, gl::VertexObject::DataType::Short,
                true, sizeof(GalaxyVtx), offsetof(GalaxyVtx, position));
            vo.addVertexBuffer(
                bo, sizeLoc, 1, gl::VertexObject::DataType::UnsignedShort,
                true, sizeof(GalaxyVtx), offsetof(GalaxyVtx, size));
            vo.addVertexBuffer(
                bo, colorLoc, 1, gl::VertexObject::DataType::UnsignedByte,
                true, sizeof(GalaxyVtx), offsetof(GalaxyVtx, colorIndex));
            vo.addVertexBuffer(
                bo, brightnessLoc, 1, gl::VertexObject::DataType::UnsignedByte,
                true, sizeof(GalaxyVtx), offsetof(GalaxyVtx, brightness));
            vo.addVertexBuffer(
                bo, CelestiaGLProgram::TextureCoord0AttributeIndex, 2, gl::VertexObject::DataType::UnsignedByte,
                true, sizeof(GalaxyVtx), offsetof(GalaxyVtx, texCoord));
            gl::Buffer io(gl::Buffer::TargetHint::ElementArray, indices);
            vo.setIndexBuffer(io, 0, gl::VertexObject::IndexType::UnsignedInt);
            m_renderDataGL2.emplace_back(std::move(bo), std::move(io), std::move(vo));
        }
        else
        {
            m_renderDataGL2.emplace_back(gl::Buffer(util::NoCreateT{}), gl::Buffer(util::NoCreateT{}), gl::VertexObject(util::NoCreateT{}));
        }
        glVertices.clear();
        indices.clear();
    }
}

struct GalaxyRenderer::RenderDataGL3
{
    RenderDataGL3(gl::Buffer &&bo, gl::VertexObject &&vo) :
        bo(std::move(bo)),
        vo(std::move(vo))
    {
    }
    gl::Buffer       bo{ util::NoCreateT{} };
    gl::VertexObject vo{ util::NoCreateT{} };
};

void
GalaxyRenderer::renderGL3()
{
    ShaderManager::GeomShaderParams params = {GL_POINTS, GL_TRIANGLE_STRIP, 4};
    CelestiaGLProgram *prog = m_renderer.getShaderManager().getShaderGL3("galaxy150", &params);
    if (prog == nullptr)
        return;

    initializeGL3(prog);

    BindTextures();

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
        prog->floatParam("minimumFeatureSize") = minimumFeatureSize;
        prog->mat4Param("m")                   = m;

        m_renderDataGL3[obj.galaxy->getFormId()].vo.draw(nPoints);
    }

    glActiveTexture(GL_TEXTURE0);
}

void
GalaxyRenderer::initializeGL3(const CelestiaGLProgram *prog)
{
    struct GalaxyVtx
    {
        Eigen::Matrix<GLshort, 3, 1>  position;
        GLushort size;       // we scale blob by size=kSpriteScaleFactor**n
        GLubyte  colorIndex; // color index [0; 255]
        GLubyte  brightness; // blob brightness [0.0; 1.0] packed as normalized byte
    };

    if (m_initialized)
        return;

    m_initialized = true;

    auto sizeLoc = prog->attribIndex("in_Size");
    auto colorLoc = prog->attribIndex("in_ColorIndex");
    auto brightnessLoc = prog->attribIndex("in_Brightness");

    const auto *gm = GalacticFormManager::get();
    std::vector<GalaxyVtx> glVertices;

    for (int count = gm->getCount(), id = 0; id < count; id++)
    {
        if (const auto* form = gm->getForm(id); form != nullptr)
        {
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
                GalaxyVtx v;
                Eigen::Vector3f p = points[i].position * std::numeric_limits<GLshort>::max();
                v.position   = p.cast<GLshort>();
                v.size       = static_cast<GLushort>(sizeFactor);
                v.colorIndex = points[i].colorIndex;
                v.brightness = points[i].brightness;

                glVertices.push_back(v);
            }

            gl::Buffer bo(gl::Buffer::TargetHint::Array, glVertices);

            gl::VertexObject vo(gl::VertexObject::Primitive::Points);

            vo.addVertexBuffer(
                bo, CelestiaGLProgram::VertexCoordAttributeIndex,
                3, gl::VertexObject::DataType::Short,
                true, sizeof(GalaxyVtx), offsetof(GalaxyVtx, position));
            vo.addVertexBuffer(
                bo, sizeLoc, 1, gl::VertexObject::DataType::UnsignedShort,
                true, sizeof(GalaxyVtx), offsetof(GalaxyVtx, size));
            vo.addVertexBuffer(
                bo, colorLoc, 1, gl::VertexObject::DataType::UnsignedByte,
                true, sizeof(GalaxyVtx), offsetof(GalaxyVtx, colorIndex));
            vo.addVertexBuffer(
                bo, brightnessLoc, 1, gl::VertexObject::DataType::UnsignedByte,
                true, sizeof(GalaxyVtx), offsetof(GalaxyVtx, brightness));

            m_renderDataGL3.emplace_back(std::move(bo), std::move(vo));
        }
        else
        {
            m_renderDataGL3.emplace_back(gl::Buffer(util::NoCreateT{}), gl::VertexObject(util::NoCreateT{}));
        }
        glVertices.clear();
    }
}

} // namespace celestia::render
