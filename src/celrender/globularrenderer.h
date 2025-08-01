// globularrenderer.h
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel, Fridger Schrempp, and Toti
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

class CelestiaGLProgram;
class Globular;
class Renderer;
class Texture;

namespace celestia::render
{

class GlobularRenderer
{
public:
    explicit GlobularRenderer(Renderer &renderer);
    ~GlobularRenderer();

    void add(const Globular *globular, const Eigen::Vector3f &offset, float brightness, float nearZ, float farZ);

    void update(const Eigen::Quaternionf &viewerOrientation, float pixelSize, float fov, float zoom);

    void render();

private:
    struct FormManager;
    struct Object;

    void renderForm(CelestiaGLProgram *tidalProg, CelestiaGLProgram *globProg, const Object &obj) const;

    // global state
    std::unique_ptr<FormManager> m_formManager;
    std::vector<Object>          m_objects;
    Renderer                    &m_renderer;

    // per-frame state
    Eigen::Quaternionf m_viewerOrientation{ Eigen::Quaternionf::Identity() };
    Eigen::Matrix3f    m_viewMat{ Eigen::Matrix3f::Identity() };
    float              m_pixelSize{ 1.0f };
    float              m_fov{ 45.0f };
    float              m_zoom{ 1.0f };
};

} // namespace celestia::render
