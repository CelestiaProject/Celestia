// openclusterrenderer.h
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel, Fridger Schrempp, and Toti
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>

class CelestiaGLProgram;
class Renderer;
class OpenCluster;

namespace celestia::render
{
class OpenClusterRenderer
{
public:
    explicit OpenClusterRenderer(Renderer &renderer);
    ~OpenClusterRenderer();

    void add(const OpenCluster *cluster, const Eigen::Vector3f &offset, float brightness, float nearZ, float farZ) const;

    void update(const Eigen::Quaternionf &viewerOrientation, float pixelSize, float fov) const;

    void render() const;

private:
    Renderer &m_renderer;
};
} // namespace celestia::render
