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

#include "objectrenderer.h"

#include <Eigen/Core>
#include <celengine/projectionmode.h>
#include <celmath/frustum.h>
#include <celrender/rendererfwd.h>

class DeepSkyObject;

class DSORenderer : public ObjectRenderer<DeepSkyObject *, double>
{
public:
    DSORenderer();

    void process(DeepSkyObject *const &, double, float) override;

    celestia::math::Frustum frustum{ celestia::math::degToRad(celestia::engine::standardFOV),
                                     1.0f,
                                     1.0f };

    Eigen::Vector3d obsPos;
    Eigen::Matrix3f orientationMatrixT;
    DSODatabase    *dsoDB{ nullptr };

    float    avgAbsMag{ 0.0f };
    uint32_t dsosProcessed{ 0 };

    celestia::render::GalaxyRenderer      *galaxyRenderer{ nullptr };
    celestia::render::GlobularRenderer    *globularRenderer{ nullptr };
    celestia::render::NebulaRenderer      *nebulaRenderer{ nullptr };
    celestia::render::OpenClusterRenderer *openClusterRenderer{ nullptr };
};
