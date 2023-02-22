// overlay.cpp
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstring>
#include <Eigen/Core>
#include <celmath/geomutil.h>
#include <celttf/truetypefont.h>
#include <celutil/color.h>
#include "overlay.h"
#include "rectangle.h"
#include "render.h"
#include "texture.h"
#include "vecgl.h"

using namespace std;
using namespace Eigen;
using namespace celmath;

Overlay::Overlay(Renderer& r) :
    renderer(r)
{
}

void Overlay::begin()
{
    projection = Ortho2D(0.0f, (float)windowWidth, 0.0f, (float)windowHeight);
    // ModelView is Identity

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    ps.depthMask = true;
    renderer.setPipelineState(ps);

    global.reset();
    useTexture = false;
}

void Overlay::end()
{
}


void Overlay::setWindowSize(int w, int h)
{
    windowWidth = w;
    windowHeight = h;
}

void Overlay::setFont(const std::shared_ptr<TextureFont>& f)
{
    if (f != font)
    {
        if (font != nullptr)
            font->flush();
        font = f;
        fontChanged = true;
    }
}


void Overlay::beginText()
{
    savePos();
    textBlock++;
    if (font != nullptr)
    {
        font->bind();
        font->setMVPMatrices(projection);
        useTexture = true;
        fontChanged = false;
    }
}

void Overlay::endText()
{
    if (textBlock > 0)
    {
        textBlock--;
        restorePos();
    }
    font->unbind();
}

void Overlay::print_impl(const std::string& s)
{
    if (font == nullptr)
        return;

    if (!useTexture || fontChanged)
    {
        font->bind();
        font->setMVPMatrices(projection);
        useTexture = true;
        fontChanged = false;
    }

    restorePos();
    auto [_, y] = font->render(s, global.x + xoffset, global.y + yoffset);
    global.y = y;
    savePos();
}

void Overlay::drawRectangle(const celestia::Rect& r)
{
    if (useTexture && r.tex == nullptr)
        useTexture = false;

    renderer.drawRectangle(r, ShaderProperties::FisheyeOverrideModeDisabled, projection);
}

void Overlay::setColor(float r, float g, float b, float a)
{
    if (font != nullptr)
        font->flush();
    glVertexAttrib4f(CelestiaGLProgram::ColorAttributeIndex, r, g, b, a);
}

void Overlay::setColor(const Color& c)
{
    if (font != nullptr)
        font->flush();
    glVertexAttrib4f(CelestiaGLProgram::ColorAttributeIndex,
                     c.red(), c.green(), c.blue(), c.alpha());
}


void Overlay::moveBy(float dx, float dy)
{
    global.x += dx;
    global.y += dy;
}

void Overlay::savePos()
{
    posStack.push_back(global);
}

void Overlay::restorePos()
{
    if (!posStack.empty())
    {
        global = posStack.back();
        posStack.pop_back();
    }
    xoffset = yoffset = 0.0f;
}
