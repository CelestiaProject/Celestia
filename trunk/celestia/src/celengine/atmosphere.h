// atmosphere.h
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_ATMOSPHERE_H_
#define _CELENGINE_ATMOSPHERE_H_

#include <celutil/reshandle.h>
#include <celutil/color.h>
#include <celengine/multitexture.h>
#include <Eigen/Core>


class Atmosphere
{
 public:
    Atmosphere() :
        height(0.0f),
        lowerColor(0.0f, 0.0f, 0.0f),
        upperColor(0.0f, 0.0f, 0.0f),
        skyColor(0.0f, 0.0f, 0.0f),
        sunsetColor(1.0f, 0.6f, 0.5f),
        cloudHeight(0.0f),
        cloudSpeed(0.0f),
        cloudTexture(),
        cloudNormalMap(),
        mieCoeff(0.0f),
        mieScaleHeight(0.0f),
        miePhaseAsymmetry(0.0f),
        rayleighCoeff(Eigen::Vector3f::Zero()),
        rayleighScaleHeight(0.0f),
        absorptionCoeff(Eigen::Vector3f::Zero()),
        cloudShadowDepth(0.0f)
    {};

 public:
    float height;
    Color lowerColor;
    Color upperColor;
    Color skyColor;
    Color sunsetColor;
        
    float cloudHeight;
    float cloudSpeed;
    MultiResTexture cloudTexture;
    MultiResTexture cloudNormalMap;
    
    float mieCoeff;
    float mieScaleHeight;
    float miePhaseAsymmetry;
    Eigen::Vector3f rayleighCoeff;
    float rayleighScaleHeight;
    Eigen::Vector3f absorptionCoeff;

    float cloudShadowDepth;
};

// Atmosphere density is modeled with a exp(-y/H) falloff, where
// H is the scale height of the atmosphere. Thus atmospheres have
// infinite extent, but we still need to choose some finite sphere
// to render. The radius of the sphere is the height at which the
// density of the atmosphere falls to the extinction threshold, i.e.
// -H * ln(extinctionThreshold)
extern const double AtmosphereExtinctionThreshold;

#endif // _CELENGINE_ATMOSPHERE_H_

