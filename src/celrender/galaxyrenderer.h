// galaxyrenderer.h
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel, Fridger Schrempp, and Toti
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <celengine/galaxyform.h>

class CelestiaGLProgram;
class Galaxy;
class Renderer;
class Texture;

namespace celestia::render
{

class GalaxyRenderer
{
public:
    explicit GalaxyRenderer(Renderer &renderer);
    ~GalaxyRenderer();

    void add(const Galaxy *galaxy, const Eigen::Vector3f &offset, float brightness, float nearZ, float farZ);

    void update(const Eigen::Quaternionf &viewerOrientation, float pixelSize, float fov, float zoom);

    void render();

private:
    struct Object;
    struct RenderData;

    bool getRenderInfo(const GalaxyRenderer::Object &obj,
                       float &brightness,
                       float &size,
                       float minimumFeatureSize,
                       Eigen::Matrix4f &m,
                       Eigen::Matrix4f &pr,
                       int &nPoints) const;

    void bindTextures();

    void renderGL2();
    void initializeGL2(const CelestiaGLProgram *prog);

    void renderGL3();
    void initializeGL3(const CelestiaGLProgram *prog);

    std::vector<RenderData>  m_renderData;

    // global state
    std::unique_ptr<Texture> m_galaxyTex;
    std::unique_ptr<Texture> m_colorTex;
    std::vector<Object>      m_objects;
    Renderer                &m_renderer;

    // per-frame state
    Eigen::Quaternionf  m_viewerOrientation{ Eigen::Quaternionf::Identity() };
    Eigen::Matrix3f     m_viewMat{ Eigen::Matrix3f::Identity() };
    float               m_pixelSize{ 1.0f };
    float               m_fov{ 45.0f };
    float               m_zoom{ 1.0f };

    bool                m_initialized{ false };
};

} // namespace celestia::render
