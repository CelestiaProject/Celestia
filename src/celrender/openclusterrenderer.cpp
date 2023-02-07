// openclusterrenderer.cpp
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel, Fridger Schrempp, and Toti
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celengine/opencluster.h>
#include <celengine/render.h>
#include "openclusterrenderer.h"


namespace celestia::render
{

OpenClusterRenderer::OpenClusterRenderer(Renderer &renderer) :
    m_renderer(renderer)
{
}

OpenClusterRenderer::~OpenClusterRenderer() = default;

void
OpenClusterRenderer::update(const Eigen::Quaternionf &/*viewerOrientation*/, float /*pixelSize*/, float /*fov*/) const
{
    // See comments in render()
}

void
OpenClusterRenderer::add(const OpenCluster * /*opencluster*/, const Eigen::Vector3f &/*offset*/, float /*brightness*/, float /*nearZ*/, float /*farZ*/) const
{
    // See comments in render()
}

void
OpenClusterRenderer::render() const
{
    // Nothing to do right now; open clusters are only visible as their
    // constituent stars and a label when labels are turned on.  A good idea
    // would be to add an 'sky chart' mode, in which clusters are rendered as
    // circles.
}

} // namespace celestia::render
