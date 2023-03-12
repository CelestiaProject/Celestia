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
#include <vector>
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
using Symbol = celestia::MarkerRepresentation::Symbol;

constexpr float pif = celestia::numbers::pi_v<float>;

constexpr int FilledSquareOffset = 0;
constexpr int FilledSquareCount  = 4;
static std::array FilledSquare =
{
    -1.0f, -1.0f,
     1.0f, -1.0f,
     1.0f,  1.0f,
    -1.0f,  1.0f
};

constexpr int RightArrowOffset = FilledSquareOffset + FilledSquareCount;
constexpr int RightArrowCount  = 9;
static std::array RightArrow =
{
    -3*1.0f,  1.0f/3.0f,
    -3*1.0f, -1.0f/3.0f,
    -2*1.0f, -1.0f/4.0f,
    -2*1.0f, -1.0f/4.0f,
    -2*1.0f,  1.0f/4.0f,
    -3*1.0f,  1.0f/3.0f,
    -2*1.0f,   2*1.0f/3,
    -2*1.0f,  -2*1.0f/3,
    -1.0f,         0.0f
};

constexpr int LeftArrowOffset = RightArrowOffset + RightArrowCount;
constexpr int LeftArrowCount  = 9;
static std::array LeftArrow =
{
     3*1.0f,    -1.0f/3,
     3*1.0f,     1.0f/3,
     2*1.0f,     1.0f/4,
     2*1.0f,     1.0f/4,
     2*1.0f,    -1.0f/4,
     3*1.0f,    -1.0f/3,
     2*1.0f,  -2*1.0f/3,
     2*1.0f,   2*1.0f/3,
     1.0f,         0.0f
};

constexpr int UpArrowOffset = LeftArrowOffset + LeftArrowCount;
constexpr int UpArrowCount  = 9;
static std::array UpArrow =
{
    -1.0f/3,    -3*1.0f,
     1.0f/3,    -3*1.0f,
     1.0f/4,    -2*1.0f,
     1.0f/4,    -2*1.0f,
    -1.0f/4,    -2*1.0f,
    -1.0f/3,    -3*1.0f,
    -2*1.0f/3,  -2*1.0f,
     2*1.0f/3,  -2*1.0f,
     0.0f,        -1.0f
};

constexpr int DownArrowOffset = UpArrowOffset + UpArrowCount;
constexpr int DownArrowCount  = 9;
static std::array DownArrow =
{
     1.0f/3,     3*1.0f,
    -1.0f/3,     3*1.0f,
    -1.0f/4,     2*1.0f,
    -1.0f/4,     2*1.0f,
     1.0f/4,     2*1.0f,
     1.0f/3,     3*1.0f,
     2*1.0f/3,   2*1.0f,
    -2*1.0f/3,   2*1.0f,
     0.0f,         1.0f
};

constexpr int SelPointerOffset = DownArrowOffset + DownArrowCount;
constexpr int SelPointerCount  = 3;
static std::array SelPointer =
{
    0.0f,       0.0f,
   -20.0f,      6.0f,
   -20.0f,     -6.0f
};

constexpr int CrosshairOffset = SelPointerOffset + SelPointerCount;
constexpr int CrosshairCount  = 3;
static std::array Crosshair =
{
    0.0f,       0.0f,
    1.0f,      -1.0f,
    1.0f,       1.0f
};


// Markers drawn with lines
constexpr int SquareCount = 8;
constexpr int SquareOffset = 0;
static std::array Square =
{
    -1.0f, -1.0f,  1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f, -1.0f
};

constexpr int TriangleOffset = SquareOffset + SquareCount;
constexpr int TriangleCount  = 6;
static std::array Triangle =
{
    0.0f,  1.0f,  1.0f, -1.0f,
    1.0f, -1.0f, -1.0f, -1.0f,
   -1.0f, -1.0f,  0.0f,  1.0f
};

constexpr int DiamondCount = 8;
constexpr int DiamondOffset = TriangleOffset + TriangleCount;
static std::array Diamond =
{
     0.0f,  1.0f,  1.0f,  0.0f,
     1.0f,  0.0f,  0.0f, -1.0f,
     0.0f, -1.0f, -1.0f,  0.0f,
    -1.0f,  0.0f,  0.0f,  1.0f

};

constexpr int PlusCount = 4;
constexpr int PlusOffset = DiamondOffset + DiamondCount;
static std::array Plus =
{
     0.0f,  1.0f,  0.0f, -1.0f,
     1.0f,  0.0f, -1.0f,  0.0f
};

constexpr int XCount = 4;
constexpr int XOffset = PlusOffset + PlusCount;
static std::array X =
{
    -1.0f, -1.0f,  1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,  1.0f
};

constexpr int StaticVtxCount = CrosshairOffset + CrosshairCount;
constexpr int SmallCircleOffset = StaticVtxCount;
constexpr int SmallCircleCount  = 10;
constexpr int LargeCircleOffset = SmallCircleOffset + SmallCircleCount;
constexpr int LargeCircleCount  = 60;

constexpr int _SmallCircleOffset = XOffset + XCount;
constexpr int _SmallCircleCount = SmallCircleCount*2;
constexpr int _LargeCircleOffset = _SmallCircleCount + _SmallCircleOffset;
constexpr int _LargeCircleCount = LargeCircleCount*2;

static std::array<float, SmallCircleCount*2> SmallCircle;
static std::array<float, LargeCircleCount*2> LargeCircle;

static void fillCircleValue(float* data, int size, float scale)
{
    float s, c;
    for (int i = 0; i < size; i++)
    {
        celmath::sincos(static_cast<float>(2 * i) / static_cast<float>(size) * pif, s, c);
        data[i * 2] = c * scale;
        data[i * 2 + 1] = s * scale;
    }
}

static void initializeCircles()
{
    static bool circlesInitilized = false;
    if (circlesInitilized)
        return;

    fillCircleValue(SmallCircle.data(), SmallCircleCount, 1.0f);
    fillCircleValue(LargeCircle.data(), LargeCircleCount, 1.0f);
    circlesInitilized = true;
}

static void initialize(gl::VertexObject &vo, gl::Buffer &bo)
{
    initializeCircles();

    std::vector<float> vertices;
    vertices.reserve((LargeCircleOffset + LargeCircleCount) * 2);

    std::copy(std::begin(FilledSquare), std::end(FilledSquare), std::back_inserter(vertices));
    std::copy(std::begin(RightArrow), std::end(RightArrow), std::back_inserter(vertices));
    std::copy(std::begin(LeftArrow), std::end(LeftArrow), std::back_inserter(vertices));
    std::copy(std::begin(UpArrow), std::end(UpArrow), std::back_inserter(vertices));
    std::copy(std::begin(DownArrow), std::end(DownArrow), std::back_inserter(vertices));
    std::copy(std::begin(SelPointer), std::end(SelPointer), std::back_inserter(vertices));
    std::copy(std::begin(Crosshair), std::end(Crosshair), std::back_inserter(vertices));
    std::copy(std::begin(SmallCircle), std::end(SmallCircle), std::back_inserter(vertices));
    std::copy(std::begin(LargeCircle), std::end(LargeCircle), std::back_inserter(vertices));

    bo.bind().setData(vertices, gl::Buffer::BufferUsage::StaticDraw);

    vo.addVertexBuffer(
        bo,
        CelestiaGLProgram::VertexCoordAttributeIndex,
        2,
        gl::VertexObject::DataType::Float);

    bo.unbind();
}

static void render_hollow_marker(const Renderer &renderer,
                                 Symbol          symbol,
                                 float           size,
                                 const Color    &color,
                                 const Matrices &m)
{
    static LineRenderer *lr = nullptr;

    if (lr == nullptr)
    {
        lr = new LineRenderer(renderer, 1.0f, LineRenderer::PrimType::Lines, LineRenderer::StorageType::Static);
        lr->setHints(LineRenderer::DISABLE_FISHEYE_TRANFORMATION);

        for (int i = 0; i < SquareCount*2; i+=2)
            lr->addVertex(Square[i], Square[i+1]);

        for (int i = 0; i < TriangleCount*2; i+=2)
            lr->addVertex(Triangle[i], Triangle[i+1]);

        for (int i = 0; i < DiamondCount*2; i+=2)
            lr->addVertex(Diamond[i], Diamond[i+1]);

        for (int i = 0; i < PlusCount*2; i+=2)
            lr->addVertex(Plus[i], Plus[i+1]);

        for (int i = 0; i < XCount*2; i+=2)
            lr->addVertex(X[i], X[i+1]);

        initializeCircles();
        for (int i = 0; i < SmallCircleCount; i++)
        {
            int j = i * 2;
            lr->addVertex(SmallCircle[j], SmallCircle[j+1]);
            int k = ((i + 1) % SmallCircleCount) * 2;
            lr->addVertex(SmallCircle[k], SmallCircle[k+1]);
        }

        for (int i = 0; i < LargeCircleCount; i++)
        {
            int j = i * 2;
            lr->addVertex(LargeCircle[j], LargeCircle[j+1]);
            int k = ((i + 1) % LargeCircleCount) * 2;
            lr->addVertex(LargeCircle[k], LargeCircle[k+1]);
        }
    }

    float s = size / 2.0f * renderer.getScaleFactor();
    Eigen::Matrix4f mv = (*m.modelview) * celmath::scale(Eigen::Vector3f(s, s, 0));

    lr->prerender();
    switch (symbol)
    {
    case MarkerRepresentation::Square:
        lr->render({m.projection, &mv}, color, SquareCount, SquareOffset);
        break;

    case MarkerRepresentation::Triangle:
        lr->render({m.projection, &mv}, color, TriangleCount, TriangleOffset);
        break;

    case MarkerRepresentation::Diamond:
        lr->render({m.projection, &mv}, color, DiamondCount, DiamondOffset);
        break;

    case MarkerRepresentation::Plus:
        lr->render({m.projection, &mv}, color, PlusCount, PlusOffset);
        break;

    case MarkerRepresentation::X:
        lr->render({m.projection, &mv}, color, XCount, XOffset);
        break;

    case MarkerRepresentation::Circle:
        if (size <= 40.0f) // TODO: this should be configurable
            lr->render({m.projection, &mv}, color, _SmallCircleCount, _SmallCircleOffset);
        else
            lr->render({m.projection, &mv}, color, _LargeCircleCount, _LargeCircleOffset);
        break;
    default:
        return;
    }
    lr->finish();
}

void
Renderer::renderMarker(Symbol symbol, float size, const Color &color, const Matrices &m)
{
    assert(shaderManager != nullptr);

    switch (symbol)
    {
    case MarkerRepresentation::Diamond:
    case MarkerRepresentation::Plus:
    case MarkerRepresentation::X:
    case MarkerRepresentation::Square:
    case MarkerRepresentation::Triangle:
    case MarkerRepresentation::Circle:
        render_hollow_marker(*this, symbol, size, color, m);
        return;
    default:
        break;
    }

    ShaderProperties shadprop;
    shadprop.texUsage = ShaderProperties::VertexColors;
    shadprop.lightModel = ShaderProperties::UnlitModel;
    shadprop.fishEyeOverride = ShaderProperties::FisheyeOverrideModeDisabled;
    auto* prog = shaderManager->getShader(shadprop);
    if (prog == nullptr)
        return;

    if (!m_markerDataInitialized)
    {
        initialize(*m_markerVO, *m_markerBO);
        m_markerDataInitialized = true;
    }

#ifdef GL_ES
    glVertexAttrib4fv(CelestiaGLProgram::ColorAttributeIndex, color.toVector4().data());
#else
    glVertexAttrib4Nubv(CelestiaGLProgram::ColorAttributeIndex, color.data());
#endif

    prog->use();
    float s = size / 2.0f * getScaleFactor();
    prog->setMVPMatrices(*m.projection, (*m.modelview) * celmath::scale(Eigen::Vector3f(s, s, 0)));

    switch (symbol)
    {
    case MarkerRepresentation::FilledSquare:
        m_markerVO->draw(gl::VertexObject::Primitive::TriangleFan, FilledSquareCount, FilledSquareOffset);
        break;

    case MarkerRepresentation::RightArrow:
        m_markerVO->draw(gl::VertexObject::Primitive::Triangles, RightArrowCount, RightArrowOffset);
        break;

    case MarkerRepresentation::LeftArrow:
        m_markerVO->draw(gl::VertexObject::Primitive::Triangles, LeftArrowCount, LeftArrowOffset);
        break;

    case MarkerRepresentation::UpArrow:
        m_markerVO->draw(gl::VertexObject::Primitive::Triangles, UpArrowCount, UpArrowOffset);
        break;

    case MarkerRepresentation::DownArrow:
        m_markerVO->draw(gl::VertexObject::Primitive::Triangles, DownArrowCount, DownArrowOffset);
        break;

    case MarkerRepresentation::Disk:
        if (size <= 40.0f) // TODO: this should be configurable
            m_markerVO->draw(gl::VertexObject::Primitive::TriangleFan, SmallCircleCount, SmallCircleOffset);
        else
            m_markerVO->draw(gl::VertexObject::Primitive::TriangleFan, LargeCircleCount, LargeCircleOffset);
        break;

    default:
        break;
    }
}

/*! Draw an arrow at the view border pointing to an offscreen selection. This method
 *  should only be called when the selection lies outside the view frustum.
 */
void Renderer::renderSelectionPointer(const Observer& observer,
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
    float h = std::tan(vfov / 2);
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
        initialize(*m_markerVO, *m_markerBO);
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
    gl::VertexObject::unbind();
}

void Renderer::renderCrosshair(float selectionSizeInPixels,
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
        initialize(*m_markerVO, *m_markerBO);
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
    gl::VertexObject::unbind();
}
