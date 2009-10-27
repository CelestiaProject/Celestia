// lightenv.h
//
// Structures that describe the lighting environment for rendering objects
// in Celestia.
//
// Copyright (C) 2006, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_LIGHTENV_H_
#define _CELENGINE_LIGHTENV_H_

#include <celutil/color.h>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <vector>

static const unsigned int MaxLights = 8;

class Body;
class RingSystem;

class DirectionalLight
{
public:
    Color color;
    float irradiance;
    Eigen::Vector3f direction_eye;
    Eigen::Vector3f direction_obj;

    // Required for eclipse shadows only--may be able to use
    // distance instead of position.
    Eigen::Vector3d position;  // position relative to the lit object
    float apparentSize;
    bool castsShadows;
};

class EclipseShadow
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    const Body* caster;
    Eigen::Quaternionf casterOrientation;
    Eigen::Vector3f origin;
    Eigen::Vector3f direction;
    float penumbraRadius;
    float umbraRadius;
    float maxDepth;
};

class RingShadow
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    RingSystem* ringSystem;
    Eigen::Quaternionf casterOrientation;
    Eigen::Vector3f origin;
    Eigen::Vector3f direction;
    float texLod;
};

class LightingState
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    typedef std::vector<EclipseShadow, Eigen::aligned_allocator<EclipseShadow> > EclipseShadowVector;

    LightingState() :
        nLights(0),
        shadowingRingSystem(NULL),
        eyeDir_obj(-Eigen::Vector3f::UnitZ()),
        eyePos_obj(-Eigen::Vector3f::UnitZ())
    {
        shadows[0] = NULL;
        for (unsigned int i = 0; i < MaxLights; ++i)
        {
            ringShadows[i].ringSystem = NULL;
        }
    };

    unsigned int nLights;
    DirectionalLight lights[MaxLights];
    EclipseShadowVector* shadows[MaxLights];
    RingShadow ringShadows[MaxLights];
    RingSystem* shadowingRingSystem; // NULL when there are no ring shadows
    Eigen::Vector3f ringPlaneNormal;
    Eigen::Vector3f ringCenter;

    Eigen::Vector3f eyeDir_obj;
    Eigen::Vector3f eyePos_obj;
    
    Eigen::Vector3f ambientColor;
};

#endif // _CELENGINE_LIGHTENV_H_
