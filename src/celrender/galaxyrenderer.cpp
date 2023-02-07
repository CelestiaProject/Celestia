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

constexpr unsigned int
RequiredIndexCount(unsigned int vCount)
{
    return ((vCount + 3) / 4) * 6;
}

struct GalaxyVertex
{
    Eigen::Vector3f position;
    Eigen::Matrix<std::uint8_t, 4, 1> texCoord; // texCoord.x = x, texCoord.y = y, texCoord.z = color index, texCoord.w = alpha
};
static_assert(std::is_standard_layout_v<GalaxyVertex>);

constexpr unsigned int MAX_VERTICES = 1024*1024 / sizeof(GalaxyVertex); // 1MB buffer
constexpr unsigned int MAX_INDICES = RequiredIndexCount(MAX_VERTICES);

void
bindVMemBuffers(std::size_t vCount)
{
    constexpr int nBuffers = 4;
    static std::array<GLuint, nBuffers> vbos = {};
    static std::array<GLuint, nBuffers> vios = {};
    static std::array<unsigned int, nBuffers> counts = {MAX_VERTICES, MAX_VERTICES / 16 , MAX_VERTICES / 64, MAX_VERTICES / 256}; // 1MB, 64kB, 16kB, 4kB

    if (static bool initialized = false; !initialized)
    {
        initialized = true;
        glGenBuffers(nBuffers, vbos.data());
        glGenBuffers(nBuffers, vios.data());
    }

    // find appropriate buffer size
    int buffer = 0;
    for(int i = nBuffers - 1; i > 0; i--)
    {
        if (vCount <= counts[i])
        {
            buffer = i;
            break;
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbos[buffer]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vios[buffer]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GalaxyVertex) * counts[buffer], nullptr, GL_STREAM_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * RequiredIndexCount(counts[buffer]), nullptr, GL_STREAM_DRAW);
}

void draw(std::size_t vCount, const GalaxyVertex *v, std::size_t iCount, const GLushort *indices)
{
    bindVMemBuffers(vCount);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GalaxyVertex) * vCount, v);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(GLushort) * iCount, indices);

    glEnableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glEnableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex);

    glVertexAttribPointer(CelestiaGLProgram::VertexCoordAttributeIndex,
                          3, GL_FLOAT, GL_FALSE,
                          sizeof(GalaxyVertex), reinterpret_cast<const void*>(offsetof(GalaxyVertex, position)));
    glVertexAttribPointer(CelestiaGLProgram::TextureCoord0AttributeIndex,
                          4, GL_UNSIGNED_BYTE, GL_TRUE,
                          sizeof(GalaxyVertex), reinterpret_cast<const void*>(offsetof(GalaxyVertex, texCoord)));

    glDrawElements(GL_TRIANGLES, iCount, GL_UNSIGNED_SHORT, nullptr);
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

struct GalaxyRenderer::RenderData
{
    RenderData(gl::Buffer &&bo, gl::VertexObject &&vo) : bo(std::move(bo)), vo(std::move(vo)) {}
    gl::Buffer       bo{ util::NoCreateT{} };
    gl::VertexObject vo{ util::NoCreateT{} };
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
GalaxyRenderer::update(const Eigen::Quaternionf &viewerOrientation, float pixelSize, float fov)
{
    m_viewerOrientation = viewerOrientation;
    m_viewMat = viewerOrientation.conjugate().toRotationMatrix();
    m_pixelSize = pixelSize;
    m_fov = fov;
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

#ifdef GL_ES
    bool useGeomShader = gl::checkVersion(gl::GLES_3_2);
#else
    bool useGeomShader = gl::checkVersion(gl::GL_3_2);
#endif

    CelestiaGLProgram *prog;
    if (useGeomShader)
    {
        ShaderManager::GeomShaderParams params = {GL_POINTS, GL_TRIANGLE_STRIP, 4};
        prog = m_renderer.getShaderManager().getShaderGL3("galaxy150", &params);
    }
    else
    {
        prog = m_renderer.getShaderManager().getShader("galaxy");
    }
    if (prog == nullptr)
        return;

    if (useGeomShader)
        initializeGL3(prog);

    BindTextures();

    prog->use();
    prog->samplerParam("galaxyTex") = 0;
    prog->samplerParam("colorTex") = 1;
    if (useGeomShader)
        prog->mat3Param("viewMat") = m_viewMat;

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    ps.smoothLines = true;
    m_renderer.setPipelineState(ps);

    for (const auto &obj : m_objects)
    {
        const auto* galacticForm = GalacticFormManager::get()->getForm(obj.galaxy->getFormId());
        if (galacticForm == nullptr)
            continue;

        // We'll first see if the galaxy's apparent size is big enough to
        // be noticeable on screen; if it's not we'll break right here,
        // avoiding all the overhead of the matrix transformations and
        // GL state changes:
        float distanceToDSO = std::max(0.0f, obj.offset.norm() - obj.galaxy->getRadius());
        float minimumFeatureSize = m_pixelSize * distanceToDSO;
        float size = 2.0f * obj.galaxy->getRadius();
        if (size < minimumFeatureSize)
            continue;

        Eigen::Matrix4f m = (
            Eigen::Translation3f(obj.offset) *
            Eigen::Affine3f(obj.galaxy->getOrientation().conjugate()) *
            Eigen::Scaling(galacticForm->scale * size)
        ).matrix();

        float brightness = obj.galaxy->getBrightnessCorrection(obj.offset) * obj.brightness;

        if (obj.nearZ != 0.0f && obj.farZ != 0.0f)
        {
            Eigen::Matrix4f pr;
            m_renderer.buildProjectionMatrix(pr, obj.nearZ, obj.farZ);
            prog->setMVPMatrices(pr, m_renderer.getModelViewMatrix());
        }
        else
        {
            prog->setMVPMatrices(m_renderer.getProjectionMatrix(), m_renderer.getModelViewMatrix());
        }

        if (useGeomShader)
        {
            prog->floatParam("size")               = size;
            prog->floatParam("brightness")         = brightness;
            prog->floatParam("minimumFeatureSize") = minimumFeatureSize;
            prog->mat4Param("m")                   = m;

            const auto &points = galacticForm->blobs;
            auto nPoints = static_cast<int>(static_cast<float>(points.size()) * std::clamp(obj.galaxy->getDetail(), 0.0f, 1.0f));
            // find proper nPoints count
            if (minimumFeatureSize > 0.0f)
            {
                auto power = static_cast<unsigned>(std::log(minimumFeatureSize/size)/std::log(kSpriteScaleFactor));
                if (power < std::numeric_limits<decltype(nPoints)>::digits)
                    nPoints = std::min(nPoints, 1 << power);
            }
            m_renderData[obj.galaxy->getFormId()].vo.draw(nPoints);
        }
        else
        {
            renderGL2(obj.galaxy, size, minimumFeatureSize, brightness, m, galacticForm->blobs);
        }
    }

    glActiveTexture(GL_TEXTURE0);
    m_objects.clear();
}

void
GalaxyRenderer::renderGL2(const Galaxy          *galaxy,
                          float                  size,
                          float                  minimumFeatureSize,
                          float                  brightness,
                          const Eigen::Matrix4f &m,
                          const BlobVector      &points) const
{
    static std::unique_ptr<GalaxyVertex[]> g_vertices{ nullptr };
    static std::unique_ptr<GLushort[]> g_indices{ nullptr };

    if (g_vertices == nullptr)
        g_vertices = std::make_unique<GalaxyVertex[]>(MAX_VERTICES);
    if (g_indices == nullptr)
        g_indices = std::make_unique<GLushort[]>(MAX_INDICES);

    Eigen::Vector3f v0 = m_viewMat * Eigen::Vector3f(-1, -1, 0) * size;
    Eigen::Vector3f v1 = m_viewMat * Eigen::Vector3f( 1, -1, 0) * size;
    Eigen::Vector3f v2 = m_viewMat * Eigen::Vector3f( 1,  1, 0) * size;
    Eigen::Vector3f v3 = m_viewMat * Eigen::Vector3f(-1,  1, 0) * size;

    auto nPoints = static_cast<unsigned>(static_cast<float>(points.size()) * std::clamp(galaxy->getDetail(), 0.0f, 1.0f));

    std::size_t vertex = 0, index = 0;
    GLushort    j = 0;
    for (unsigned int i = 0, pow2 = 1; i < nPoints; ++i)
    {
        if ((i & pow2) != 0)
        {
            pow2 <<= 1;
            size *= kSpriteScaleFactor;
            v0 *= kSpriteScaleFactor;
            v1 *= kSpriteScaleFactor;
            v2 *= kSpriteScaleFactor;
            v3 *= kSpriteScaleFactor;
            if (size < minimumFeatureSize)
                break;
        }

        const auto &b = points[i];
        Eigen::Vector3f p = (m * Eigen::Vector4f(b.position.x(), b.position.y(), b.position.z(), 1.0f)).head(3);

        float screenFrac = size / p.norm();
        if (screenFrac < 0.1f)
        {
            float    a = std::min(255.0f, (0.1f - screenFrac) * b.brightness * brightness);
            auto alpha = static_cast<std::uint8_t>(a); // encode as byte
            g_vertices[vertex++] = { p + v0, { std::uint8_t(0),   std::uint8_t(0),   b.colorIndex, alpha } };
            g_vertices[vertex++] = { p + v1, { std::uint8_t(255), std::uint8_t(0),   b.colorIndex, alpha } };
            g_vertices[vertex++] = { p + v2, { std::uint8_t(255), std::uint8_t(255), b.colorIndex, alpha } };
            g_vertices[vertex++] = { p + v3, { std::uint8_t(0),   std::uint8_t(255), b.colorIndex, alpha } };

            g_indices[index++] = j;
            g_indices[index++] = j + 1;
            g_indices[index++] = j + 2;
            g_indices[index++] = j;
            g_indices[index++] = j + 2;
            g_indices[index++] = j + 3;
            j += 4;

            if (vertex + 4 > MAX_VERTICES)
            {
                draw(vertex, g_vertices.get(), index, g_indices.get());
                index  = 0;
                vertex = 0;
                j      = 0;
            }
        }
    }

    if (index > 0)
        draw(vertex, g_vertices.get(), index, g_indices.get());

    glDisableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glDisableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
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

    auto s = prog->attribIndex("in_Size");
    auto c = prog->attribIndex("in_ColorIndex");
    auto b = prog->attribIndex("in_Brightness");

    const auto *gm = GalacticFormManager::get();
    std::vector<GalaxyVtx> glVertices;

    assert(gm->getCount() == m_renderData.size());

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
                bo, s, 1, gl::VertexObject::DataType::UnsignedShort,
                true, sizeof(GalaxyVtx), offsetof(GalaxyVtx, size));
            vo.addVertexBuffer(
                bo, c, 1, gl::VertexObject::DataType::UnsignedByte,
                true, sizeof(GalaxyVtx), offsetof(GalaxyVtx, colorIndex));
            vo.addVertexBuffer(
                bo, b, 1, gl::VertexObject::DataType::UnsignedByte,
                true, sizeof(GalaxyVtx), offsetof(GalaxyVtx, brightness));

            m_renderData.emplace_back(std::move(bo), std::move(vo));
        }
        else
        {
            m_renderData.emplace_back(gl::Buffer(util::NoCreateT{}), gl::VertexObject(util::NoCreateT{}));
        }
        glVertices.clear();
    }
}

} // namespace celestia::render
