// glmarker.cpp
//
// Copyright (C) 2019, Celestia Development Team
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "marker.h"
#include <celmath/mathlib.h>
#include <GL/glew.h>
#include <vector>
#include "render.h"


using namespace std;
using namespace celgl;
using namespace celmath;
using namespace Eigen;


constexpr const int DiamondOffset = 0;
constexpr const int DiamondCount  = 4;
static GLfloat Diamond[DiamondCount * 2] =
{
     0.0f,  1.0f,
     1.0f,  0.0f,
     0.0f, -1.0f,
    -1.0f,  0.0f
};

constexpr const int PlusOffset = DiamondOffset + DiamondCount;
constexpr const int PlusCount  = 4;
static GLfloat Plus[PlusCount * 2] =
{
     0.0f,  1.0f,
     0.0f, -1.0f,
     1.0f,  0.0f,
    -1.0f,  0.0f
};

constexpr const int XOffset = PlusOffset + PlusCount;
constexpr const int XCount  = 4;
static GLfloat X[XCount * 2] =
{
    -1.0f, -1.0f,
     1.0f,  1.0f,
     1.0f, -1.0f,
    -1.0f,  1.0f
};

constexpr const int SquareOffset = XOffset + XCount;
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

constexpr const int StaticVtxCount = DownArrowOffset + DownArrowCount;

constexpr const int SmallCircleOffset = DownArrowOffset + DownArrowCount;
static int SmallCircleCount  = 0;
static int LargeCircleOffset = 0;
static int LargeCircleCount  = 0;

static void initVO(VertexObject& vo)
{
    vector<GLfloat> small, large;
    for (int i = 0; i < 360; i += 36)
    {
        float c, s;
        sincos(degToRad(static_cast<float>(i)), s, c);
        small.push_back(c); small.push_back(s);
        large.push_back(c); large.push_back(s);
        for (int j = i+6; j < i+36; j += 6)
        {
            sincos(degToRad(static_cast<float>(j)), s, c);
            large.push_back(c); large.push_back(s);
        }
    };

    SmallCircleCount = small.size() / 2;
    LargeCircleCount = large.size() / 2;
    LargeCircleOffset = SmallCircleOffset + SmallCircleCount;

#define VTXTOMEM(a) ((a) * sizeof(GLfloat) * 2)
    vo.allocate(VTXTOMEM(StaticVtxCount + SmallCircleCount + LargeCircleCount));

#define VOSTREAM(a) vo.setBufferData(a, VTXTOMEM(a ## Offset), sizeof(a))
    VOSTREAM(Diamond);
    VOSTREAM(Plus);
    VOSTREAM(X);
    VOSTREAM(Square);
    VOSTREAM(Triangle);
    VOSTREAM(RightArrow);
    VOSTREAM(LeftArrow);
    VOSTREAM(UpArrow);
    VOSTREAM(DownArrow);
#undef VOSTREAM

    vo.setBufferData(small.data(), VTXTOMEM(SmallCircleOffset), small.size() * sizeof(GLfloat));
    vo.setBufferData(large.data(), VTXTOMEM(LargeCircleOffset), large.size() * sizeof(GLfloat));
#undef VTXTOMEM

    vo.setVertices(2, GL_FLOAT, false, 0, 0);
}

void Renderer::renderMarker(MarkerRepresentation::Symbol symbol, float size, const Color& color)
{
    markerVO.bind();

    if (!markerVO.initialized())
        initVO(markerVO);

    assert(shaderManager != nullptr);
    auto* prog = shaderManager->getShader("marker");
    if (prog == nullptr)
        return;

    float s = size / 2.0f;
    prog->use();
    prog->vec4Param("color") = color.toVector4();
    prog->floatParam("s") = s;

    switch (symbol)
    {
    case MarkerRepresentation::Diamond:
        markerVO.draw(GL_LINE_LOOP, DiamondCount, DiamondOffset);
        break;

    case MarkerRepresentation::Plus:
        markerVO.draw(GL_LINES, PlusCount, PlusOffset);
        break;

    case MarkerRepresentation::X:
        markerVO.draw(GL_LINES, XCount, XOffset);
        break;

    case MarkerRepresentation::Square:
        markerVO.draw(GL_LINE_LOOP, SquareCount, SquareOffset);
        break;

    case MarkerRepresentation::FilledSquare:
        markerVO.draw(GL_TRIANGLE_FAN, SquareCount, SquareOffset);
        break;

    case MarkerRepresentation::Triangle:
        markerVO.draw(GL_LINE_LOOP, TriangleCount, TriangleOffset);
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
        if (s <= 20)
            markerVO.draw(GL_LINE_LOOP, SmallCircleCount, SmallCircleOffset);
        else
            markerVO.draw(GL_LINE_LOOP, LargeCircleCount, LargeCircleOffset);
        break;

    case MarkerRepresentation::Disk:
        if (s <= 20) // TODO: this should be configurable
            markerVO.draw(GL_TRIANGLE_FAN, SmallCircleCount, SmallCircleOffset);
        else
            markerVO.draw(GL_TRIANGLE_FAN, LargeCircleCount, LargeCircleOffset);
        break;

    default:
        break;
    }

    glUseProgram(0);
    markerVO.unbind();
}
