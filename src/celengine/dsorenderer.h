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

namespace celestia::engine::detail
{

double dsoIrradianceScale(double distanceLightYears);
float dsoDisplayBrightness(double logReferenceIrradiance, double irradianceScale,
                           double averageLogIrradiance, double minimumLogIrradiance, float correction);

} // namespace celestia::engine::detail

class DSORenderer : public ObjectRenderer<std::unique_ptr<DeepSkyObject>, double>
{
public:
    DSORenderer();

    void process(const std::unique_ptr<DeepSkyObject>&, double, float) override; //NOSONAR

    celestia::math::InfiniteFrustum frustum{ celestia::math::degToRad(celestia::engine::standardFOV),
                                             1.0f,
                                             1.0f };

    Eigen::Vector3d obsPos;
    Eigen::Matrix3f orientationMatrixT;
    DSODatabase    *dsoDB{ nullptr };

    double averageLogIrradiance{ 0.0 };
    double minimumLogIrradiance{ 0.0 };
    float brightnessCorrection{ 0.0f };
    float labelResponseScale{ 0.0f };
    std::uint32_t dsosProcessed{ 0 };

    celestia::render::GalaxyRenderer      *galaxyRenderer{ nullptr };
    celestia::render::GlobularRenderer    *globularRenderer{ nullptr };
    celestia::render::NebulaRenderer      *nebulaRenderer{ nullptr };
    celestia::render::OpenClusterRenderer *openClusterRenderer{ nullptr };
};
