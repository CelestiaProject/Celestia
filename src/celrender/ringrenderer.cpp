// ringrenderer.cpp
//
// Copyright (C) 2006-2024, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "ringrenderer.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include <Eigen/Core>

#include <celcompat/numbers.h>
#include <celengine/body.h>
#include <celengine/lightenv.h>
#include <celengine/multitexture.h>
#include <celengine/render.h>
#include <celengine/renderinfo.h>
#include <celengine/shadermanager.h>
#include <celengine/texture.h>
#include <celmath/mathlib.h>

namespace celestia::render
{

namespace
{

constexpr float SegmentSizeThreshold = 30.0f;
constexpr std::uint32_t BaseSectionCount = UINT32_C(180);

struct RingVertex
{
    std::array<float, 3>  pos;
    std::array<unsigned short, 2> tex;
};

ShaderProperties
createShaderProperties(const LightingState& ls,
                       const Texture* ringsTex,
                       bool renderShadow)
{
    // Set up the shader properties for ring rendering
    ShaderProperties shadprop;
    shadprop.lightModel = LightingModel::RingIllumModel;
    shadprop.nLights = static_cast<std::uint16_t>(std::min(ls.nLights, MaxShaderLights));

    if (renderShadow)
    {
        // Set one shadow (the planet's) per light
        for (unsigned int li = 0; li < ls.nLights; li++)
            shadprop.setEclipseShadowCountForLight(li, 1);
    }

    if (ringsTex != nullptr)
        shadprop.texUsage = TexUsage::DiffuseTexture;

    return shadprop;
}

void
setUpShadowParameters(CelestiaGLProgram* prog,
                      const LightingState& ls,
                      float planetOblateness)
{
    for (unsigned int li = 0; li < ls.nLights; li++)
    {
        const DirectionalLight& light = ls.lights[li];

        // Compute the projection vectors based on the sun direction.
        // I'm being a little careless here--if the sun direction lies
        // along the y-axis, this will fail.  It's unlikely that a
        // planet would ever orbit underneath its sun (an orbital
        // inclination of 90 degrees), but this should be made
        // more robust anyway.
        Eigen::Vector3f axis = Eigen::Vector3f::UnitY().cross(light.direction_obj);
        float cosAngle = Eigen::Vector3f::UnitY().dot(light.direction_obj);
        axis.normalize();

        float tScale = 1.0f;
        if (planetOblateness != 0.0f)
        {
            // For oblate planets, the size of the shadow volume will vary
            // based on the light direction.

            // A vertical slice of the planet is an ellipse
            float a = 1.0f;                          // semimajor axis
            float b = a * (1.0f - planetOblateness); // semiminor axis
            float ecc2 = 1.0f - (b * b) / (a * a);   // square of eccentricity

            // Calculate the radius of the ellipse at the incident angle of the
            // light on the ring plane + 90 degrees.
            float r = a * std::sqrt((1.0f - ecc2) /
                                    (1.0f - ecc2 * math::square(cosAngle)));

            tScale *= a / r;
        }

        // The s axis is perpendicular to the shadow axis in the plane of the
        // of the rings, and the t axis completes the orthonormal basis.
        Eigen::Vector3f sAxis = axis * 0.5f;
        Eigen::Vector3f tAxis = (axis.cross(light.direction_obj)) * 0.5f * tScale;
        Eigen::Vector4f texGenS;
        texGenS.head(3) = sAxis;
        texGenS[3] = 0.5f;
        Eigen::Vector4f texGenT;
        texGenT.head(3) = tAxis;
        texGenT[3] = 0.5f;

        // r0 and r1 determine the size of the planet's shadow and penumbra
        // on the rings.
        // A more accurate ring shadow calculation would set r1 / r0
        // to the ratio of the apparent sizes of the planet and sun as seen
        // from the rings. Even more realism could be attained by letting
        // this ratio vary across the rings, though it may not make enough
        // of a visual difference to be worth the extra effort.
        float r0 = 0.24f;
        float r1 = 0.25f;
        float bias = 1.0f / (1.0f - r1 / r0);

        prog->shadows[li][0].texGenS = texGenS;
        prog->shadows[li][0].texGenT = texGenT;
        prog->shadows[li][0].maxDepth = 1.0f;
        prog->shadows[li][0].falloff = bias / r0;
    }
}

} // end unnamed namespace

RingRenderer::RingRenderer(Renderer& _renderer) : renderer(_renderer)
{
    // Initialize section scales
    std::uint32_t nSections = BaseSectionCount;
    for (unsigned int i = 0; i < RingRenderer::nLODs - 1; ++i)
    {
        sectionScales[i] = static_cast<float>(std::tan(celestia::numbers::pi / static_cast<double>(nSections)));
        nSections <<= 1;
    }
}

// Render a planetary ring system
void
RingRenderer::renderRings(RingSystem& rings,
                          const RenderInfo& ri,
                          const LightingState& ls,
                          float planetRadius,
                          float planetOblateness,
                          bool renderShadow,
                          float segmentSizeInPixels,
                          const Matrices &m,
                          bool inside)
{
    float inner = rings.innerRadius / planetRadius;
    float outer = rings.outerRadius / planetRadius;
    Texture* ringsTex = rings.texture.find(renderer.getResolution());

    ShaderProperties shadprop = createShaderProperties(ls, ringsTex, renderShadow);

    // Get a shader for the current rendering configuration
    auto* prog = renderer.getShaderManager().getShader(shadprop);
    if (prog == nullptr)
        return;

    prog->use();
    prog->setMVPMatrices(*m.projection, *m.modelview);

    prog->eyePosition = ls.eyePos_obj;
    prog->ambientColor = ri.ambientColor.toVector3();
    prog->setLightParameters(ls, ri.color, ri.specularColor, Color::Black);

    prog->ringRadius = inner;
    prog->ringWidth = outer - inner;

    setUpShadowParameters(prog, ls, planetOblateness);

    if (ringsTex != nullptr)
        ringsTex->bind();

    // Determine level of detail
    std::uint32_t nSections = BaseSectionCount;
    unsigned int level = 0;
    for (level = 0U; level < nLODs - 1U; ++level)
    {
        if (float s = segmentSizeInPixels * sectionScales[level]; s < SegmentSizeThreshold)
            break;

        nSections <<= 1;
    }

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    ps.depthTest = true;
    ps.depthMask = inside;
    renderer.setPipelineState(ps);

    renderLOD(level, nSections);
}

void
RingRenderer::initializeLOD(unsigned int level, std::uint32_t nSections)
{
    std::vector<RingVertex> ringCoord;
    ringCoord.reserve(2 * nSections);

    constexpr float angle = 2.0f * celestia::numbers::pi_v<float>;
    for (std::uint32_t i = 0; i <= nSections; i++)
    {
        float theta = angle * static_cast<float>(i) / static_cast<float>(nSections);
        float s, c;
        math::sincos(theta, s, c);

        RingVertex vertex;
        // inner point
        vertex.pos[0] = c;
        vertex.pos[1] = 0.0f;
        vertex.pos[2] = s;
        vertex.tex[0] = 0;
        vertex.tex[1] = 0;
        ringCoord.push_back(vertex);

        // outer point
        vertex.tex[0] = 1;

        ringCoord.push_back(vertex);
    }

    const auto& bo = buffers[level].emplace(gl::Buffer::TargetHint::Array, ringCoord);
    auto& vo = vertexObjects[level].emplace(gl::VertexObject::Primitive::TriangleStrip);
    vo.setCount(static_cast<int>((nSections + 1) * 2))
      .addVertexBuffer(bo,
                       CelestiaGLProgram::TextureCoord0AttributeIndex,
                       2,
                       gl::VertexObject::DataType::UnsignedShort,
                       false,
                       sizeof(RingVertex),
                       offsetof(RingVertex, tex))
      .addVertexBuffer(bo,
                       CelestiaGLProgram::VertexCoordAttributeIndex,
                       3,
                       gl::VertexObject::DataType::Float,
                       false,
                       sizeof(RingVertex),
                       offsetof(RingVertex, pos));
    bo.unbind();
}

void
RingRenderer::renderLOD(unsigned int level, std::uint32_t nSections)
{
    if (!vertexObjects[level].has_value())
        initializeLOD(level, nSections);
    glDisable(GL_CULL_FACE);
    vertexObjects[level]->draw();
    glEnable(GL_CULL_FACE);
}

} // end namespace celestia::render
