// glmarker.cpp
//
// Copyright (C) 2019, Celestia Development Team
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <vector>
#include <celcompat/numbers.h>
#include <celmath/frustum.h>
#include <celmath/mathlib.h>
#include <celrender/linerenderer.h>
#include <celrender/vertexobject.h>
#include "marker.h"
#include "render.h"
#include "vecgl.h"


using namespace std;
using namespace celmath;
using namespace celestia;
using namespace Eigen;
using celestia::render::LineRenderer;
using celestia::render::VertexObject;

constexpr int FilledSquareOffset = 0;
constexpr int FilledSquareCount  = 4;
static GLfloat FilledSquare[FilledSquareCount * 2] =
{
    -1.0f, -1.0f,
     1.0f, -1.0f,
     1.0f,  1.0f,
    -1.0f,  1.0f
};

constexpr int RightArrowOffset = FilledSquareOffset + FilledSquareCount;
constexpr int RightArrowCount  = 9;
static GLfloat RightArrow[RightArrowCount * 2] =
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
static GLfloat LeftArrow[LeftArrowCount * 2] =
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
static GLfloat UpArrow[UpArrowCount * 2] =
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
static GLfloat DownArrow[DownArrowCount * 2] =
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
static GLfloat SelPointer[SelPointerCount * 2] =
{
    0.0f,       0.0f,
   -20.0f,      6.0f,
   -20.0f,     -6.0f
};

constexpr int CrosshairOffset = SelPointerOffset + SelPointerCount;
constexpr int CrosshairCount  = 3;
static GLfloat Crosshair[CrosshairCount * 2] =
{
    0.0f,       0.0f,
    1.0f,      -1.0f,
    1.0f,       1.0f
};


// Markers drawn with lines
constexpr int SquareCount = 8;
constexpr int SquareOffset = 0;
static GLfloat Square[SquareCount * 2] =
{
    -1.0f, -1.0f,  1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f, -1.0f
};

constexpr int TriangleOffset = SquareOffset + SquareCount;
constexpr int TriangleCount  = 6;
static GLfloat Triangle[TriangleCount * 2] =
{
    0.0f,  1.0f,  1.0f, -1.0f,
    1.0f, -1.0f, -1.0f, -1.0f,
   -1.0f, -1.0f,  0.0f,  1.0f
};

constexpr int DiamondCount = 8;
constexpr int DiamondOffset = TriangleOffset + TriangleCount;
static GLfloat Diamond[DiamondCount * 2] =
{
     0.0f,  1.0f,  1.0f,  0.0f,
     1.0f,  0.0f,  0.0f, -1.0f,
     0.0f, -1.0f, -1.0f,  0.0f,
    -1.0f,  0.0f,  0.0f,  1.0f

};

constexpr int PlusCount = 4;
constexpr int PlusOffset = DiamondOffset + DiamondCount;
static GLfloat Plus[PlusCount * 2] =
{
     0.0f,  1.0f,  0.0f, -1.0f,
     1.0f,  0.0f, -1.0f,  0.0f
};

constexpr int XCount = 4;
constexpr int XOffset = PlusOffset + PlusCount;
static GLfloat X[XCount * 2] =
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

static GLfloat SmallCircle[SmallCircleCount*2];
static GLfloat LargeCircle[LargeCircleCount*2];

static bool CirclesInitilized = false;

static void fillCircleValue(GLfloat* data, int size, float scale)
{
    float s, c;
    for (int i = 0; i < size; i++)
    {
        sincos((float) (2 * i) / (float) size * celestia::numbers::pi_v<float>, s, c);
        data[i * 2] = c * scale;
        data[i * 2 + 1] = s * scale;
    }
}

static int bufferVertices(VertexObject& vo, const GLfloat* data, int vertexCount, int& offset)
{
    int dataSize = vertexCount * 2 * sizeof(GLfloat);
    vo.setBufferData(data, offset, dataSize);
    offset += dataSize;
    return vertexCount;
}

static void initializeCircles()
{
    if (CirclesInitilized)
        return;

    fillCircleValue(SmallCircle, SmallCircleCount, 1.0f);
    fillCircleValue(LargeCircle, LargeCircleCount, 1.0f);
    CirclesInitilized = true;
}

static void initVO(VertexObject& vo)
{
    initializeCircles();

    vo.allocate((LargeCircleOffset + LargeCircleCount) * sizeof(GLfloat) * 2);

    int offset = 0;
#define VOSTREAM(a) bufferVertices(vo, a, a##Count, offset)
    VOSTREAM(FilledSquare);
    VOSTREAM(RightArrow);
    VOSTREAM(LeftArrow);
    VOSTREAM(UpArrow);
    VOSTREAM(DownArrow);
    VOSTREAM(SelPointer);
    VOSTREAM(Crosshair);
    VOSTREAM(SmallCircle);
    VOSTREAM(LargeCircle);
#undef VOSTREAM

    vo.setVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex,
                            2, GL_FLOAT, false, 0, 0);
}


static void render_hollow_marker(const Renderer &renderer,
                        celestia::MarkerRepresentation::Symbol symbol,
                        float size,
                        const Color &color,
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
    Eigen::Matrix4f mv = (*m.modelview) * vecgl::scale(Vector3f(s, s, 0));

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

void Renderer::renderMarker(celestia::MarkerRepresentation::Symbol symbol,
                            float size,
                            const Color &color,
                            const Matrices &m)
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

    auto &markerVO = getVertexObject(VOType::Marker, GL_ARRAY_BUFFER, 0, GL_STATIC_DRAW);
    markerVO.bind();
    if (!markerVO.initialized())
        initVO(markerVO);

#ifdef GL_ES
    glVertexAttrib4fv(CelestiaGLProgram::ColorAttributeIndex, color.toVector4().data());
#else
    glVertexAttrib4Nubv(CelestiaGLProgram::ColorAttributeIndex, color.data());
#endif

    prog->use();
    float s = size / 2.0f * getScaleFactor();
    prog->setMVPMatrices(*m.projection, (*m.modelview) * vecgl::scale(Vector3f(s, s, 0)));

    switch (symbol)
    {
    case MarkerRepresentation::FilledSquare:
        markerVO.draw(GL_TRIANGLE_FAN, FilledSquareCount, FilledSquareOffset);
        break;

    case MarkerRepresentation::RightArrow:
        markerVO.draw(GL_TRIANGLES, RightArrowCount, RightArrowOffset);
        break;

    case MarkerRepresentation::LeftArrow:
        markerVO.draw(GL_TRIANGLES, LeftArrowCount, LeftArrowOffset);
        break;

    case MarkerRepresentation::UpArrow:
        markerVO.draw(GL_TRIANGLES, UpArrowCount, UpArrowOffset);
        break;

    case MarkerRepresentation::DownArrow:
        markerVO.draw(GL_TRIANGLES, DownArrowCount, DownArrowOffset);
        break;

    case MarkerRepresentation::Disk:
        if (size <= 40.0f) // TODO: this should be configurable
            markerVO.draw(GL_TRIANGLE_FAN, SmallCircleCount, SmallCircleOffset);
        else
            markerVO.draw(GL_TRIANGLE_FAN, LargeCircleCount, LargeCircleOffset);
        break;

    default:
        break;
    }

    markerVO.unbind();
}

/*! Draw an arrow at the view border pointing to an offscreen selection. This method
 *  should only be called when the selection lies outside the view frustum.
 */
void Renderer::renderSelectionPointer(const Observer& observer,
                                      double now,
                                      const Frustum& viewFrustum,
                                      const Selection& sel)
{
    constexpr float cursorDistance = 20.0f;
    if (sel.empty())
        return;

    // Get the position of the cursor relative to the eye
    Vector3d position = sel.getPosition(now).offsetFromKm(observer.getPosition());
    if (viewFrustum.testSphere(position, sel.radius()) != Frustum::Outside)
        return;

    assert(shaderManager != nullptr);
    auto* prog = shaderManager->getShader("selpointer");
    if (prog == nullptr)
        return;

    Matrix3f cameraMatrix = getCameraOrientation().conjugate().toRotationMatrix();
    const Vector3f u = cameraMatrix.col(0);
    const Vector3f v = cameraMatrix.col(1);
    double distance = position.norm();
    position *= cursorDistance / distance;

    float vfov = observer.getFOV();
    float h = tan(vfov / 2);
    float w = h * getAspectRatio();
    float diag = sqrt(h * h + w * w);

    Vector3f posf = position.cast<float>() / cursorDistance;
    float x = u.dot(posf);
    float y = v.dot(posf);
    float c, s;
    sincos(atan2(y, x), s, c);

    float x0 = c * diag;
    float y0 = s * diag;
    float t = (std::abs(x0) < w) ? h / abs(y0) : w / abs(x0);
    x0 *= t;
    y0 *= t;

    auto &markerVO = getVertexObject(VOType::Marker, GL_ARRAY_BUFFER, 0, GL_STATIC_DRAW);
    markerVO.bind();
    if (!markerVO.initialized())
        initVO(markerVO);

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    ps.depthMask = true;
    setPipelineState(ps);

    prog->use();
    const Vector3f &center = cameraMatrix.col(2);
    prog->setMVPMatrices(getProjectionMatrix(), getModelViewMatrix() * vecgl::translate(Vector3f(-center)));
    prog->vec4Param("color") = Color(SelectionCursorColor, 0.6f).toVector4();
    prog->floatParam("pixelSize") = pixelSize * getScaleFactor();
    prog->floatParam("s") = s;
    prog->floatParam("c") = c;
    prog->floatParam("x0") = x0;
    prog->floatParam("y0") = y0;
    prog->vec3Param("u") = u;
    prog->vec3Param("v") = v;
    markerVO.draw(GL_TRIANGLES, SelPointerCount, SelPointerOffset);
    markerVO.unbind();
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

    auto &markerVO = getVertexObject(VOType::Marker, GL_ARRAY_BUFFER, 0, GL_STATIC_DRAW);
    markerVO.bind();
    if (!markerVO.initialized())
        initVO(markerVO);

    const float cursorMinRadius = 6.0f;
    const float cursorRadiusVariability = 4.0f;
    const float minCursorWidth = 7.0f;
    const float cursorPulsePeriod = 1.5f;

    float cursorRadius = selectionSizeInPixels + cursorMinRadius;
    cursorRadius += cursorRadiusVariability *
                    (float) (0.5 + 0.5 * sin(tsec * 2 * celestia::numbers::pi / cursorPulsePeriod));

    // Enlarge the size of the cross hair sligtly when the selection
    // has a large apparent size
    float cursorGrow = max(1.0f, min(2.5f, (selectionSizeInPixels - 10.0f) / 100.0f));

    prog->use();
    prog->setMVPMatrices(*m.projection, *m.modelview);
    prog->vec4Param("color") = color.toVector4();
    prog->floatParam("radius") = cursorRadius;
    float scaleFactor = getScaleFactor();
    prog->floatParam("width") = minCursorWidth * cursorGrow * scaleFactor;
    prog->floatParam("h") = 2.0f * cursorGrow * scaleFactor;

    const unsigned int markCount = 4;
    for (unsigned int i = 0; i < markCount; i++)
    {
        float theta = (celestia::numbers::pi_v<float> / 4.0f) + (float) i / (float) markCount * (2.0f * celestia::numbers::pi_v<float>);
        prog->floatParam("angle") = theta;
        markerVO.draw(GL_TRIANGLES, CrosshairCount, CrosshairOffset);
    }
    markerVO.unbind();
}
