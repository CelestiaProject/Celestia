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
#include <celutil/color.h>
#include "overlay.h"
#include "rectangle.h"
#include "render.h"
#include "textlayout.h"

using namespace std;
using namespace Eigen;
using namespace celmath;
using namespace celestia::engine;

Overlay::Overlay(Renderer& r) :
    layout(make_unique<TextLayout>(r.getScreenDpi())),
    renderer(r)
{
}

void Overlay::begin()
{
    layout->setLayoutDirectionFollowTextAlignment(true);
    layout->setScreenDpi(renderer.getScreenDpi());

    projection = Ortho2D(0.0f, (float)windowWidth, 0.0f, (float)windowHeight);
    // ModelView is Identity

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    ps.depthMask = true;
    renderer.setPipelineState(ps);
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
    layout->setFont(f);
}

void Overlay::setTextAlignment(TextLayout::HorizontalAlignment halign)
{
    layout->setHorizontalAlignment(halign);
}

void Overlay::beginText()
{
    savePos();
    layout->begin(projection);
}

void Overlay::endText()
{
    layout->end();
    restorePos();
}

void Overlay::print_impl(const std::string& s)
{
    layout->render(s);
}

void Overlay::drawRectangle(const celestia::Rect& r)
{
    renderer.drawRectangle(r, ShaderProperties::FisheyeOverrideModeDisabled, projection);
}

void Overlay::setColor(float r, float g, float b, float a)
{
    layout->flush();
    glVertexAttrib4f(CelestiaGLProgram::ColorAttributeIndex, r, g, b, a);
}

void Overlay::setColor(const Color& c)
{
    layout->flush();
    glVertexAttrib4f(CelestiaGLProgram::ColorAttributeIndex,
                     c.red(), c.green(), c.blue(), c.alpha());
}

void Overlay::moveBy(float dx, float dy)
{
    layout->moveRelative(dx, dy);
}

void Overlay::moveBy(int dx, int dy)
{
    layout->moveRelative(static_cast<float>(dx), static_cast<float>(dy));
}

void Overlay::savePos()
{
    posStack.push_back(layout->getCurrentPosition());
}

void Overlay::restorePos()
{
    if (!posStack.empty())
    {
        auto [x, y] = posStack.back();
        layout->moveAbsolute(x, y);
        posStack.pop_back();
    }
}
