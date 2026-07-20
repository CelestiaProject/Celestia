// atmosphere.h
//
// Copyright (C) 2001-2025, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <Eigen/Core>

#include <celutil/color.h>
#include <celutil/texhandle.h>

struct Atmosphere
{
    float height { 0.0f };
    Color lowerColor;
    Color upperColor;
    Color skyColor;
    Color sunsetColor{ 1.0f, 0.6f, 0.5f };

    float cloudHeight{ 0.0f };
    float cloudSpeed{ 0.0f };
    celestia::util::TextureHandle cloudTexture{ celestia::util::TextureHandle::Invalid };
    celestia::util::TextureHandle cloudNormalMap{ celestia::util::TextureHandle::Invalid };

    float mieCoeff{ 0.0f };
    float mieScaleHeight{ 0.0f };
    float miePhaseAsymmetry{ 0.0f };
    Eigen::Vector3f rayleighCoeff{ Eigen::Vector3f::Zero() };
    float rayleighScaleHeight{ 0.0f };
    Eigen::Vector3f absorptionCoeff{ Eigen::Vector3f::Zero() };

    float cloudShadowDepth{ 0.0f };
};
