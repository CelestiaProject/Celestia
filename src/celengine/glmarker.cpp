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
#include "marker.h"
#include "render.h"
#include "vecgl.h"
#include "vertexobject.h"


using namespace std;
using namespace celgl;
using namespace celmath;
using namespace celestia;
using namespace Eigen;

constexpr const int SquareOffset = 0;
constexpr const int SquareCount  = 4;
static GLfloat Square[SquareCount * 2] =
{
    -1.0f, -1.0f,
     1.0f, -1.0f,
     1.0f,  1.0f,
    -1.0f,  1.0f
};

constexpr const int TriangleOffset = SquareOffset + SquareCount;
constexpr const int TriangleCount  = 3;
static GLfloat Triangle[TriangleCount * 2] =
{
    0.0f,  1.0f,
    1.0f, -1.0f,
   -1.0f, -1.0f
};

constexpr const int RightArrowOffset = TriangleOffset + TriangleCount;
constexpr const int RightArrowCount  = 9;
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

constexpr const int LeftArrowOffset = RightArrowOffset + RightArrowCount;
constexpr const int LeftArrowCount  = 9;
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

constexpr const int UpArrowOffset = LeftArrowOffset + LeftArrowCount;
constexpr const int UpArrowCount  = 9;
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

constexpr const int DownArrowOffset = UpArrowOffset + UpArrowCount;
constexpr const int DownArrowCount  = 9;
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

constexpr const int SelPointerOffset = DownArrowOffset + DownArrowCount;
constexpr const int SelPointerCount  = 3;
static GLfloat SelPointer[SelPointerCount * 2] =
{
    0.0f,       0.0f,
   -20.0f,      6.0f,
   -20.0f,     -6.0f
};

constexpr const int CrosshairOffset = SelPointerOffset + SelPointerCount;
constexpr const int CrosshairCount  = 3;
static GLfloat Crosshair[CrosshairCount * 2] =
{
    0.0f,       0.0f,
    1.0f,      -1.0f,
    1.0f,       1.0f
};

constexpr const int StaticVtxCount = CrosshairOffset + CrosshairCount;

constexpr const int SmallCircleOffset = StaticVtxCount;
constexpr const int SmallCircleCount  = 10;
constexpr const int LargeCircleOffset = SmallCircleOffset + SmallCircleCount;
constexpr const int LargeCircleCount  = 60;

static int DiamondLineOffset = 0;
static int DiamondLineCount = 0;
static int PlusLineOffset = 0;
static int PlusLineCount = 0;
static int XLineOffset = 0;
static int XLineCount = 0;
static int TriangleLineOffset = 0;
static int TriangleLineCount = 0;
static int SquareLineOffset = 0;
static int SquareLineCount = 0;
static int SmallCircleLineOffset = 0;
static int SmallCircleLineCount = 0;
static int LargeCircleLineOffset = 0;
static int LargeCircleLineCount = 0;
static int EclipticLineCount = 0;

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

static int bufferVertices(VertexObject& vo, GLfloat* data, int vertexCount, int& offset)
{
    int dataSize = vertexCount * 2 * sizeof(GLfloat);
    vo.setBufferData(data, offset, dataSize);
    offset += dataSize;
    return vertexCount;
}

static int bufferLineVertices(VertexObject& vo, GLfloat* data, int vertexSize, int vertexCount, int& offset)
{
    int dataSize = vertexCount * 3 * (2 * vertexSize + 1) * sizeof(GLfloat);
    GLfloat* tranformed = new GLfloat[dataSize];
    GLfloat* ptr = tranformed;
    for (int i = 0; i < vertexCount; i += 2)
    {
        int index = i * vertexSize;
        GLfloat* thisVert = &data[index];
        GLfloat* nextVert = &data[index + vertexSize];
#define STREAM_AND_ADVANCE(firstVertex, secondVertex, scale) do {\
memcpy(ptr, firstVertex, vertexSize * sizeof(GLfloat));\
memcpy(ptr + vertexSize, secondVertex, vertexSize * sizeof(GLfloat));\
ptr[2 * vertexSize] = scale;\
ptr += 2 * vertexSize + 1;\
} while (0)
        STREAM_AND_ADVANCE(thisVert, nextVert, -0.5);
        STREAM_AND_ADVANCE(thisVert, nextVert,  0.5);
        STREAM_AND_ADVANCE(nextVert, thisVert, -0.5);
        STREAM_AND_ADVANCE(nextVert, thisVert, -0.5);
        STREAM_AND_ADVANCE(nextVert, thisVert,  0.5);
        STREAM_AND_ADVANCE(thisVert, nextVert, -0.5);
#undef STREAM_AND_ADVANCE
    }
    vo.setBufferData(tranformed, offset, dataSize);
    offset += dataSize;
    delete[] tranformed;
    return vertexCount * 3;
}

static int bufferLineLoopVertices(VertexObject& vo, GLfloat* data, int vertexSize, int vertexCount, int& offset)
{
    int dataSize = vertexCount * 6 * (2 * vertexSize + 1) * sizeof(GLfloat);
    GLfloat* tranformed = new GLfloat[dataSize];
    GLfloat* ptr = tranformed;
    for (int i = 0; i < vertexCount; i += 1)
    {
        int index = i * vertexSize;
        int nextIndex = ((i + 1) % vertexCount) * vertexSize;
        GLfloat* thisVert = &data[index];
        GLfloat* nextVert = &data[nextIndex];
#define STREAM_AND_ADVANCE(firstVertex, secondVertex, scale) do {\
memcpy(ptr, firstVertex, vertexSize * sizeof(GLfloat));\
memcpy(ptr + vertexSize, secondVertex, vertexSize * sizeof(GLfloat));\
ptr[2 * vertexSize] = scale;\
ptr += 2 * vertexSize + 1;\
} while (0)
        STREAM_AND_ADVANCE(thisVert, nextVert, -0.5);
        STREAM_AND_ADVANCE(thisVert, nextVert,  0.5);
        STREAM_AND_ADVANCE(nextVert, thisVert, -0.5);
        STREAM_AND_ADVANCE(nextVert, thisVert, -0.5);
        STREAM_AND_ADVANCE(nextVert, thisVert,  0.5);
        STREAM_AND_ADVANCE(thisVert, nextVert, -0.5);
#undef STREAM_AND_ADVANCE
    }
    vo.setBufferData(tranformed, offset, dataSize);
    offset += dataSize;
    delete[] tranformed;
    return vertexCount * 6;
}

static void initVO(VertexObject& vo)
{
    GLfloat SmallCircle[SmallCircleCount * 2];
    GLfloat LargeCircle[LargeCircleCount * 2];
    fillCircleValue(SmallCircle, SmallCircleCount, 1.0f);
    fillCircleValue(LargeCircle, LargeCircleCount, 1.0f);

    vo.allocate((LargeCircleOffset + LargeCircleCount) * sizeof(GLfloat) * 2);

    int offset = 0;
#define VOSTREAM(a) bufferVertices(vo, a, a##Count, offset)
    VOSTREAM(Square);
    VOSTREAM(Triangle);
    VOSTREAM(RightArrow);
    VOSTREAM(LeftArrow);
    VOSTREAM(UpArrow);
    VOSTREAM(DownArrow);
    VOSTREAM(SelPointer);
    VOSTREAM(Crosshair);
    VOSTREAM(SmallCircle);
    VOSTREAM(LargeCircle);
#undef VOSTREAM

    vo.setVertices(2, GL_FLOAT, false, 0, 0);
}

static void initLineVO(VertexObject& vo)
{
    constexpr const int DiamondCount  = 4;
    static GLfloat Diamond[DiamondCount * 2] =
    {
         0.0f,  1.0f,
         1.0f,  0.0f,
         0.0f, -1.0f,
        -1.0f,  0.0f
    };
    constexpr const int PlusCount  = 4;
    GLfloat Plus[PlusCount * 2] =
    {
         0.0f,  1.0f,
         0.0f, -1.0f,
         1.0f,  0.0f,
        -1.0f,  0.0f
    };
    constexpr const int XCount  = 4;
    GLfloat X[XCount * 2] =
    {
        -1.0f, -1.0f,
         1.0f,  1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f
    };

    GLfloat SmallCircle[SmallCircleCount * 2];
    GLfloat LargeCircle[LargeCircleCount * 2];
    fillCircleValue(SmallCircle, SmallCircleCount, 1.0f);
    fillCircleValue(LargeCircle, LargeCircleCount, 1.0f);

    int stride = sizeof(GLfloat) * 5;
    int size = ((DiamondCount + SquareCount + TriangleCount + SmallCircleCount + LargeCircleCount) * 6 + (PlusCount + XCount) * 3) * stride;

    vo.allocate(size);

    int offset = 0;
#define VOSTREAM_LINES(a) do { \
a##LineOffset = offset / stride / 6;\
a##LineCount = bufferLineVertices(vo, a, 2, a##Count, offset) / 6; \
} while (0)
#define VOSTREAM_LINE_LOOP(a) do { \
a##LineOffset = offset / stride / 6;\
a##LineCount = bufferLineLoopVertices(vo, a, 2, a##Count, offset) / 6; \
} while (0)
    VOSTREAM_LINE_LOOP(Diamond);
    VOSTREAM_LINES(Plus);
    VOSTREAM_LINES(X);
    VOSTREAM_LINE_LOOP(Square);
    VOSTREAM_LINE_LOOP(Triangle);
    VOSTREAM_LINE_LOOP(SmallCircle);
    VOSTREAM_LINE_LOOP(LargeCircle);
#undef VOSTREAM_LINES
#undef VOSTREAM_LINE_LOOP

    vo.setVertices(2, GL_FLOAT, false, stride, 0);
    vo.setVertexAttribArray(CelestiaGLProgram::NextVCoordAttributeIndex, 2, GL_FLOAT, false, stride, sizeof(GLfloat) * 2);
    vo.setVertexAttribArray(CelestiaGLProgram::ScaleFactorAttributeIndex, 1, GL_FLOAT, false, stride, sizeof(GLfloat) * 4);

    vo.setVertices(2, GL_FLOAT, false, stride * 3, 0, VertexObject::AttributesType::Alternative1);
}

static void initEclipticVO(VertexObject& vo)
{
    constexpr const int eclipticCount = 200;
    GLfloat ecliptic[eclipticCount * 3];
    float s, c;
    float scale = 1000.0f;
    for (int i = 0; i < eclipticCount; i++)
    {
        sincos((float) (2 * i) / (float) eclipticCount * celestia::numbers::pi_v<float>, s, c);
        ecliptic[i * 3] = c * scale;
        ecliptic[i * 3 + 1] = 0;
        ecliptic[i * 3 + 2] = s * scale;
    }

    int stride = sizeof(GLfloat) * 7;
    vo.allocate(eclipticCount * 6 * stride);
    int offset = 0;
    EclipticLineCount = bufferLineLoopVertices(vo, ecliptic, 3, eclipticCount, offset) / 6;

    vo.setVertices(3, GL_FLOAT, false, stride, 0);
    vo.setVertexAttribArray(CelestiaGLProgram::NextVCoordAttributeIndex, 3, GL_FLOAT, false, stride, sizeof(GLfloat) * 3);
    vo.setVertexAttribArray(CelestiaGLProgram::ScaleFactorAttributeIndex, 1, GL_FLOAT, false, stride, sizeof(GLfloat) * 6);

    vo.setVertices(3, GL_FLOAT, false, stride * 3, 0, VertexObject::AttributesType::Alternative1);
}

void Renderer::renderMarker(celestia::MarkerRepresentation::Symbol symbol,
                            float size,
                            const Color &color,
                            const Matrices &m)
{
    assert(shaderManager != nullptr);

    using namespace celestia;
    using AttributesType = celgl::VertexObject::AttributesType;

    bool solid = true;
    switch (symbol)
    {
    case MarkerRepresentation::Diamond:
    case MarkerRepresentation::Plus:
    case MarkerRepresentation::X:
    case MarkerRepresentation::Square:
    case MarkerRepresentation::Triangle:
    case MarkerRepresentation::Circle:
        solid = false;
    default:
        break;
    }

    ShaderProperties shadprop;
    shadprop.texUsage = ShaderProperties::VertexColors;

    bool lineAsTriangles = false;
    if (!solid)
    {
        lineAsTriangles = shouldDrawLineAsTriangles();
        if (lineAsTriangles)
            shadprop.texUsage |= ShaderProperties::LineAsTriangles;
    }
    shadprop.lightModel = ShaderProperties::UnlitModel;
    shadprop.fishEyeOverride = ShaderProperties::FisheyeOverrideModeDisabled;
    auto* prog = shaderManager->getShader(shadprop);
    if (prog == nullptr)
        return;

    auto &markerVO = getVertexObject(solid ? VOType::Marker : VOType::MarkerLine, GL_ARRAY_BUFFER, 0, GL_STATIC_DRAW);
    if (solid)
        markerVO.bind();
    else
        markerVO.bind(lineAsTriangles ? AttributesType::Default : AttributesType::Alternative1);
    if (!markerVO.initialized())
        solid ? initVO(markerVO) : initLineVO(markerVO);

    glVertexAttrib(CelestiaGLProgram::ColorAttributeIndex, color);

    prog->use();
    float s = size / 2.0f * getScaleFactor();
    prog->setMVPMatrices(*m.projection, (*m.modelview) * vecgl::scale(Vector3f(s, s, 0)));
    if (lineAsTriangles)
    {
        prog->lineWidthX = getLineWidthX();
        prog->lineWidthY = getLineWidthY();
    }

#define DRAW_LINE(type) do { \
if (lineAsTriangles) \
    markerVO.draw(GL_TRIANGLES, type##LineCount * 6, type##LineOffset * 6); \
else \
    markerVO.draw(GL_LINES, type##LineCount * 2, type##LineOffset * 2); \
} while (0)
    switch (symbol)
    {
    case MarkerRepresentation::Diamond:
        DRAW_LINE(Diamond);
        break;

    case MarkerRepresentation::Plus:
        DRAW_LINE(Plus);
        break;

    case MarkerRepresentation::X:
        DRAW_LINE(X);
        break;

    case MarkerRepresentation::Square:
        DRAW_LINE(Square);
        break;

    case MarkerRepresentation::FilledSquare:
        markerVO.draw(GL_TRIANGLE_FAN, SquareCount, SquareOffset);
        break;

    case MarkerRepresentation::Triangle:
        DRAW_LINE(Triangle);
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
        markerVO.draw(GL_TRIANGLES, UpArrowCount, DownArrowOffset);
        break;

    case MarkerRepresentation::Circle:
        if (size <= 40.0f) // TODO: this should be configurable
            DRAW_LINE(SmallCircle);
        else
            DRAW_LINE(LargeCircle);
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
#undef DRAW_LINE

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
    constexpr const float cursorDistance = 20.0f;
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

    float vfov = (float) observer.getFOV();
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

/*! Draw the J2000.0 ecliptic; trivial, since this forms the basis for
 *  Celestia's coordinate system.
 */
void Renderer::renderEclipticLine()
{
    using AttributesType = celgl::VertexObject::AttributesType;

    ShaderProperties shadprop;
    shadprop.texUsage = ShaderProperties::VertexColors;
    shadprop.lightModel = ShaderProperties::UnlitModel;

    bool lineAsTriangles = shouldDrawLineAsTriangles();
    if (lineAsTriangles)
        shadprop.texUsage |= ShaderProperties::LineAsTriangles;

    auto* prog = shaderManager->getShader(shadprop);
    if (prog == nullptr)
        return;

    auto &eclipticVO = getVertexObject(VOType::Ecliptic, GL_ARRAY_BUFFER, 0, GL_STATIC_DRAW);
    eclipticVO.bind(lineAsTriangles ? AttributesType::Default : AttributesType::Alternative1);
    if (!eclipticVO.initialized())
        initEclipticVO(eclipticVO);

    glVertexAttrib(CelestiaGLProgram::ColorAttributeIndex, EclipticColor);

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    ps.smoothLines = true;
    setPipelineState(ps);

    prog->use();
    prog->setMVPMatrices(getProjectionMatrix(), getModelViewMatrix());
    if (lineAsTriangles)
    {
        prog->lineWidthX = getLineWidthX();
        prog->lineWidthY = getLineWidthY();
        eclipticVO.draw(GL_TRIANGLES, EclipticLineCount * 6);
    }
    else
    {
        eclipticVO.draw(GL_LINES, EclipticLineCount * 2);
    }

    eclipticVO.unbind();
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
        float theta = (float) (celestia::numbers::pi / 4.0) + (float) i / (float) markCount * (float) (2 * celestia::numbers::pi);
        prog->floatParam("angle") = theta;
        markerVO.draw(GL_TRIANGLES, CrosshairCount, CrosshairOffset);
    }
    markerVO.unbind();
}
