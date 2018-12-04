// boundaries.cpp
//
// Copyright (C) 2002-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include <numeric>
#include <GL/glew.h>
#include "boundaries.h"
#include "astro.h"
#include "render.h"

using namespace Eigen;
using namespace std;

constexpr const float BoundariesDrawDistance = 10000.0f;

ConstellationBoundaries::ConstellationBoundaries()
{
    currentChain = new Chain();
    currentChain->emplace_back(Vector3f::Zero());

    shadprop.staticShader = true;
    shadprop.staticProps  = ShaderProperties::UniformColor;
}

ConstellationBoundaries::~ConstellationBoundaries()
{
    cleanup();
}

void ConstellationBoundaries::cleanup()
{
    for (const auto chain : chains)
        delete chain;
    chains.clear();

    delete currentChain;
    currentChain = nullptr;

    delete[] vtx_buf;
    vtx_buf = nullptr;
}


void ConstellationBoundaries::moveto(float ra, float dec)
{
    assert(currentChain != nullptr);

    Vector3f v = astro::equatorialToEclipticCartesian(ra, dec, BoundariesDrawDistance);
    if (currentChain->size() > 1)
    {
        chains.emplace_back(currentChain);
        currentChain = new Chain();
        currentChain->emplace_back(v);
    }
    else
    {
        (*currentChain)[0] = v;
    }
}


void ConstellationBoundaries::lineto(float ra, float dec)
{
    currentChain->emplace_back(astro::equatorialToEclipticCartesian(ra, dec, BoundariesDrawDistance));
}


void ConstellationBoundaries::render(const Color& color, const Renderer& renderer)
{
    if (vboId == 0)
    {
        if (!prepared)
            prepare();

        glGenBuffers(1, &(vboId));
        glBindBuffer(GL_ARRAY_BUFFER, vboId);
        glBufferData(GL_ARRAY_BUFFER, vtx_num * 3 * sizeof(GLshort), vtx_buf, GL_STATIC_DRAW);
        cleanup();
    }
    else
    {
        glBindBuffer(GL_ARRAY_BUFFER, vboId);
    }

    CelestiaGLProgram* prog = renderer.getShaderManager().getShader(shadprop);
    if (prog == nullptr)
        return;

    prog->use();
    prog->color = color.toVector4();
    glEnableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glVertexAttribPointer(CelestiaGLProgram::VertexCoordAttributeIndex, 3, GL_SHORT, GL_FALSE, 0, 0);
    glDrawArrays(GL_LINES, 0, vtx_num);
    glDisableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);

    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void ConstellationBoundaries::prepare()
{
    vtx_num = accumulate(chains.begin(), chains.end(), 0,
                         [](int a, Chain* b) { return a + b->size(); });

    // as we use GL_LINES we should double the number of vertices
    vtx_num *= 2;

    vtx_buf = new GLshort[vtx_num * 3];
    GLshort* ptr = vtx_buf;
    for (const auto chain : chains)
    {
        for (unsigned j = 0; j < 3; j++, ptr++)
            *ptr = (GLshort) (*chain)[0][j];
        for (unsigned i = 1; i < chain->size(); i++)
        {
            for (unsigned j = 0; j < 3; j++)
                ptr[j] = ptr[j + 3] = (GLshort) (*chain)[i][j];
            ptr += 6;
        }
        for (unsigned j = 0; j < 3; j++, ptr++)
            *ptr = (GLshort) (*chain)[0][j];
    }

    prepared = true;
}


ConstellationBoundaries* ReadBoundaries(istream& in)
{
    auto* boundaries = new ConstellationBoundaries();
    string lastCon;
    int conCount = 0;
    int ptCount = 0;

    for (;;)
    {
        float ra = 0.0f;
        float dec = 0.0f;
        in >> ra;
        if (!in.good())
            break;
        in >> dec;

        string pt;
        string con;

        in >> con;
        in >> pt;
        if (!in.good())
            break;

        if (con != lastCon)
        {
            boundaries->moveto(ra, dec);
            lastCon = con;
            conCount++;
        }
        else
        {
            boundaries->lineto(ra, dec);
        }
        ptCount++;
    }

    return boundaries;
}
