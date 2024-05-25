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
#include "objectrenderer.h"
#include "projectionmode.h"

class DeepSkyObject;
class DSODatabase;
class Observer;
class Renderer;

class DSORenderer : public ObjectRenderer<double>
{
public:
    DSORenderer(const Observer*,
                Renderer*,
                const DSODatabase*,
                float _minNearPlaneDistance,
                float _labelThresholdMag);

    void process(const std::unique_ptr<DeepSkyObject>&) const;

private:
    void renderDSO(const DeepSkyObject*, const Eigen::Vector3f&, double, double, float) const;
    void labelDSO(const DeepSkyObject*, const Eigen::Vector3f&, unsigned int, double, float) const;

    Renderer* renderer;
    celestia::render::GalaxyRenderer* galaxyRenderer;
    celestia::render::GlobularRenderer* globularRenderer;
    celestia::render::NebulaRenderer* nebulaRenderer;
    celestia::render::OpenClusterRenderer* openClusterRenderer;
    const DSODatabase *dsoDB;
    std::uint64_t renderFlags;
    unsigned int labelMode;
    float labelThresholdMag;
    float avgAbsMag;
    float pixelSize;
    celestia::math::InfiniteFrustum frustum;
    Eigen::Matrix3f orientationMatrixT;
};
