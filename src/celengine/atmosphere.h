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

#include <config.h>

#ifdef HAVE_CONSTEXPR_CMATH
#include <cmath>
#endif

#include <Eigen/Core>

#include <celutil/color.h>
#include <celengine/multitexture.h>

struct Atmosphere
{
    float height { 0.0f };
    Color lowerColor;
    Color upperColor;
    Color skyColor;
    Color sunsetColor{ 1.0f, 0.6f, 0.5f };

    float cloudHeight{ 0.0f };
    float cloudSpeed{ 0.0f };
    MultiResTexture cloudTexture;
    MultiResTexture cloudNormalMap;

    float mieCoeff{ 0.0f };
    float mieScaleHeight{ 0.0f };
    float miePhaseAsymmetry{ 0.0f };
    Eigen::Vector3f rayleighCoeff{ Eigen::Vector3f::Zero() };
    float rayleighScaleHeight{ 0.0f };
    Eigen::Vector3f absorptionCoeff{ Eigen::Vector3f::Zero() };

    float cloudShadowDepth{ 0.0f };
};

// Atmosphere density is modeled with a exp(-y/H) falloff, where
// H is the scale height of the atmosphere. Thus atmospheres have
// infinite extent, but we still need to choose some finite sphere
// to render. The radius of the sphere is the height at which the
// density of the atmosphere falls to the extinction threshold, i.e.
// -H * ln(extinctionThreshold)
constexpr inline float AtmosphereExtinctionThreshold = 0.05f;

#ifdef HAVE_CONSTEXPR_CMATH
constexpr inline float LogAtmosphereExtinctionThreshold = std::log(AtmosphereExtinctionThreshold);
#else
extern const float LogAtmosphereExtinctionThreshold;
#endif
