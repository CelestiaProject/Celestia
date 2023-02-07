// nebularenderer.cpp
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel, Fridger Schrempp, and Toti
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celengine/meshmanager.h>
#include <celengine/nebula.h>
#include <celengine/rendcontext.h>
#include <celengine/render.h>
#include <celmath/geomutil.h>
#include <celmath/vecgl.h>
#include <celutil/reshandle.h>
#include "nebularenderer.h"


namespace celestia::render
{

struct NebulaRenderer::Object
{
    Object(const Eigen::Vector3f &offset, float nearZ, float farZ, const Nebula *nebula) :
        offset(offset),
        nearZ(nearZ),
        farZ(farZ),
        nebula(nebula)
    {
    }

    Eigen::Vector3f offset; // distance to the nebula
    float           nearZ;  // if nearZ != & farZ != then use custom projection matrix
    float           farZ;
    const Nebula   *nebula;
};

NebulaRenderer::NebulaRenderer(Renderer &renderer) :
    m_renderer(renderer)
{
}

NebulaRenderer::~NebulaRenderer() = default; // define here as Object is not defined in the header file

void
NebulaRenderer::update(const Eigen::Quaternionf &viewerOrientation, float pixelSize, float fov)
{
    m_viewerOrientation = viewerOrientation;
    m_pixelSize = pixelSize;
    m_fov = fov;
}

void
NebulaRenderer::add(const Nebula *nebula, const Eigen::Vector3f &offset, float /*brightness*/, float nearZ, float farZ)
{
    m_objects.emplace_back(offset, nearZ, farZ, nebula);
}

void
NebulaRenderer::render()
{
    for (const auto &obj : m_objects)
        renderNebula(obj);

    m_objects.clear();
}

void
NebulaRenderer::renderNebula(const Object &obj) const
{
    Geometry *g = nullptr;
    if (auto geometry = obj.nebula->getGeometry(); geometry != InvalidResource)
        g = GetGeometryManager()->find(geometry);
    if (g == nullptr)
        return;

    Eigen::Matrix4f pr;
    if (obj.nearZ != 0.0f && obj.farZ != 0.0f)
        m_renderer.buildProjectionMatrix(pr, obj.nearZ, obj.farZ);
    else
        pr = m_renderer.getProjectionMatrix();

    Renderer::PipelineState ps;
    ps.smoothLines = true;
    m_renderer.setPipelineState(ps);

    float radius = obj.nebula->getRadius();

    Eigen::Matrix4f mv = celmath::rotate(
        celmath::scale(celmath::translate(m_renderer.getModelViewMatrix(), obj.offset), radius),
        obj.nebula->getOrientation());

    GLSLUnlit_RenderContext rc(&m_renderer, radius, &mv, &pr);
    rc.setPointScale(2.0f * radius / m_pixelSize);
    g->render(rc);
}

} // namespace celestia::render
