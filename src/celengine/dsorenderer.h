// dsorenderer.h
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>
#include <memory>

#include <Eigen/Core>

#include <celmath/frustum.h>
#include <celmath/mathlib.h>
#include <celrender/rendererfwd.h>
#include "projectionmode.h"
#include "visibleobjectvisitor.h"

class DeepSkyObject;
class DSODatabase;
class Observer;
class Renderer;

class DSORenderer : private VisibleObjectVisitor<double>
{
public:
    DSORenderer(const Observer*,
                Renderer*,
                const DSODatabase*,
                float minNearPlaneDistance,
                float labelThresholdMag);

    using VisibleObjectVisitor::checkNode;
    void process(const std::unique_ptr<DeepSkyObject>&) const;

private:
    void renderDSO(const DeepSkyObject*, const Eigen::Vector3f&, double, double, float) const;
    void labelDSO(const DeepSkyObject*, const Eigen::Vector3f&, unsigned int, double, float) const;

    Renderer* m_renderer;
    celestia::render::GalaxyRenderer* m_galaxyRenderer;
    celestia::render::GlobularRenderer* m_globularRenderer;
    celestia::render::NebulaRenderer* m_nebulaRenderer;
    celestia::render::OpenClusterRenderer* m_openClusterRenderer;
    const DSODatabase* m_dsoDB;
    std::uint64_t m_renderFlags;
    unsigned int m_labelMode;
    float m_labelThresholdMag;
    float m_avgAbsMag;
    float m_pixelSize;
    celestia::math::InfiniteFrustum m_frustum;
    Eigen::Matrix3f m_orientationMatrixT;
};
