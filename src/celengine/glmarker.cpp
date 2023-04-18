// glmarker.cpp
//
// Copyright (C) 2019, Celestia Development Team
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <array>
#include <celcompat/numbers.h>
#include <celmath/frustum.h>
#include <celmath/mathlib.h>
#include <celmath/vecgl.h>
#include <celrender/gl/buffer.h>
#include <celrender/gl/vertexobject.h>
#include <celrender/linerenderer.h>
#include "marker.h"
#include "render.h"


using namespace celestia;
using celestia::render::LineRenderer;

namespace
{
constexpr float pif = celestia::numbers::pi_v<float>;

#include "markers.inc"

void
initialize(LineRenderer &lr, gl::VertexObject &vo, gl::Buffer &bo)
{

    bo.bind().setData(FilledMarkersData, gl::Buffer::BufferUsage::StaticDraw);

    vo.addVertexBuffer(
        bo,
        CelestiaGLProgram::VertexCoordAttributeIndex,
        2,
        gl::VertexObject::DataType::Float);

    bo.unbind();

    lr.setHints(LineRenderer::DISABLE_FISHEYE_TRANFORMATION);

    for (int i = 0; i < HollowMarkersData.size(); i+=2)
        lr.addVertex(HollowMarkersData[i], HollowMarkersData[i+1]);
}

}

void
Renderer::renderMarker(MarkerRepresentation::Symbol symbol,
                       float size,
                       const Color &color,
                       const Matrices &m)
{
    assert(shaderManager != nullptr);

    if (!m_markerDataInitialized)
    {
        initialize(*m_hollowMarkerRenderer, *m_markerVO, *m_markerBO);
        m_markerDataInitialized = true;
    }

    float s = size / 2.0f * getScaleFactor();
    Eigen::Matrix4f mv = (*m.modelview) * celmath::scale(Eigen::Vector3f(s, s, 0));

    switch (symbol)
    {
    case MarkerRepresentation::Diamond:
    case MarkerRepresentation::Plus:
    case MarkerRepresentation::X:
    case MarkerRepresentation::Square:
    case MarkerRepresentation::Triangle:
    case MarkerRepresentation::Circle:
        RenderHollowMarker(*m_hollowMarkerRenderer, symbol, size, color, {m.projection, &mv});
        break;
    case MarkerRepresentation::FilledSquare:
    case MarkerRepresentation::RightArrow:
    case MarkerRepresentation::LeftArrow:
    case MarkerRepresentation::UpArrow:
    case MarkerRepresentation::DownArrow:
    case MarkerRepresentation::Disk:
        RenderFilledMarker(*this, *m_markerVO, symbol, size, color, {m.projection, &mv});
        break;
    default:
        break;
    }
}

/*! Draw an arrow at the view border pointing to an offscreen selection. This method
 *  should only be called when the selection lies outside the view frustum.
 */
void
Renderer::renderSelectionPointer(const Observer& observer,
                                 double now,
                                 const celmath::Frustum& viewFrustum,
                                 const Selection& sel)
{
    constexpr float cursorDistance = 20.0f;
    if (sel.empty())
        return;

    // Get the position of the cursor relative to the eye
    Eigen::Vector3d position = sel.getPosition(now).offsetFromKm(observer.getPosition());
    if (viewFrustum.testSphere(position, sel.radius()) != celmath::Frustum::Outside)
        return;

    assert(shaderManager != nullptr);
    auto* prog = shaderManager->getShader("selpointer");
    if (prog == nullptr)
        return;

    Eigen::Matrix3f cameraMatrix = getCameraOrientation().conjugate().toRotationMatrix();
    Eigen::Vector3f u = cameraMatrix.col(0);
    Eigen::Vector3f v = cameraMatrix.col(1);
    double distance = position.norm();
    position *= cursorDistance / distance;

    float vfov = observer.getFOV();
    float h = std::tan(vfov / 2.0f);
    float w = h * getAspectRatio();
    float diag = std::hypot(h, w);

    Eigen::Vector3f posf = position.cast<float>() / cursorDistance;
    float x = u.dot(posf);
    float y = v.dot(posf);
    float c, s;
    celmath::sincos(std::atan2(y, x), s, c);

    float x0 = c * diag;
    float y0 = s * diag;
    float t = (std::abs(x0) < w) ? h / std::abs(y0) : w / std::abs(x0);
    x0 *= t;
    y0 *= t;

    if (!m_markerDataInitialized)
    {
        initialize(*m_hollowMarkerRenderer, *m_markerVO, *m_markerBO);
        m_markerDataInitialized = true;
    }

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    ps.depthMask = true;
    setPipelineState(ps);

    prog->use();
    Eigen::Vector3f center = cameraMatrix.col(2);
    prog->setMVPMatrices(getProjectionMatrix(), getModelViewMatrix() * celmath::translate(Eigen::Vector3f(-center)));
    prog->vec4Param("color") = Color(SelectionCursorColor, 0.6f).toVector4();
    prog->floatParam("pixelSize") = pixelSize * getScaleFactor();
    prog->floatParam("s") = s;
    prog->floatParam("c") = c;
    prog->floatParam("x0") = x0;
    prog->floatParam("y0") = y0;
    prog->vec3Param("u") = u;
    prog->vec3Param("v") = v;
    m_markerVO->draw(gl::VertexObject::Primitive::Triangles, SelPointerCount, SelPointerOffset);
}

void
Renderer::renderCrosshair(float selectionSizeInPixels,
                          double tsec,
                          const Color &color,
                          const Matrices &m)
{
    assert(shaderManager != nullptr);
    auto* prog = shaderManager->getShader("crosshair");
    if (prog == nullptr)
        return;

    if (!m_markerDataInitialized)
    {
        initialize(*m_hollowMarkerRenderer, *m_markerVO, *m_markerBO);
        m_markerDataInitialized = true;
    }

    const float cursorMinRadius = 6.0f;
    const float cursorRadiusVariability = 4.0f;
    const float minCursorWidth = 7.0f;
    const float cursorPulsePeriod = 1.5f;

    float cursorRadius = selectionSizeInPixels + cursorMinRadius;
    const float tsecf = static_cast<float>(tsec);
    cursorRadius += cursorRadiusVariability
                    * (0.5f + 0.5f * std::sin(tsecf * 2.0f * pif / cursorPulsePeriod));

    // Enlarge the size of the cross hair sligtly when the selection
    // has a large apparent size
    float cursorGrow = std::clamp((selectionSizeInPixels - 10.0f) / 100.0f, 1.0f, 2.5f);

    prog->use();
    prog->setMVPMatrices(*m.projection, *m.modelview);
    prog->vec4Param("color") = color.toVector4();
    prog->floatParam("radius") = cursorRadius;
    float scaleFactor = getScaleFactor();
    prog->floatParam("width") = minCursorWidth * cursorGrow * scaleFactor;
    prog->floatParam("h") = 2.0f * cursorGrow * scaleFactor;

    for (unsigned int markCount = 4, i = 0; i < markCount; i++)
    {
        float theta = (pif / 4.0f) + (2.0f * pif) * static_cast<float>(i) / static_cast<float>(markCount);
        prog->floatParam("angle") = theta;
        m_markerVO->draw(gl::VertexObject::Primitive::Triangles, CrosshairCount, CrosshairOffset);
    }
}
