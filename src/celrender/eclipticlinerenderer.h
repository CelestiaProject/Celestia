// eclipticlinerenderer.h
//
// Copyright (C) 2001-present, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include "linerenderer.h"

class Renderer;

namespace celestia::render
{
/*! Draw the J2000.0 ecliptic; trivial, since this forms the basis for
 *  Celestia's coordinate system.
 */
class EclipticLineRenderer
{
public:
    EclipticLineRenderer(Renderer &renderer);
    ~EclipticLineRenderer() = default;

    void render();

private:
    void init();

    Renderer    &m_renderer;
    LineRenderer m_lineRenderer;
    bool         m_initialized  { false };
};
} // namespace celestia::render
