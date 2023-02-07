// nebularrenderer.h
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
class Nebula;

namespace celestia::render
{
class NebulaRenderer
{
public:
    explicit NebulaRenderer(Renderer &renderer);
    ~NebulaRenderer();

    void add(const Nebula *nebula, const Eigen::Vector3f &offset, float brightness, float nearZ, float farZ);

    void update(const Eigen::Quaternionf &viewerOrientation, float pixelSize, float fov);

    void render();

private:
    struct Object;

    void renderNebula(const Object &obj) const;

    // global state
    std::vector<Object> m_objects;
    Renderer           &m_renderer;

    // per-frame state
    Eigen::Quaternionf  m_viewerOrientation{ Eigen::Quaternionf::Identity() };
    float               m_pixelSize{ 1.0f };
    float               m_fov{ 45.0f };
};
} // namespace celestia::render
