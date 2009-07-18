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
#include <vector>

static const unsigned int MaxLights = 8;

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
    Eigen::Vector3f origin;
    Eigen::Vector3f direction;
    float penumbraRadius;
    float umbraRadius;
    float maxDepth;
};

class LightingState
{
public:
    LightingState() :
        nLights(0),
        eyeDir_obj(-Eigen::Vector3f::UnitZ()),
        eyePos_obj(-Eigen::Vector3f::UnitZ())
    { shadows[0] = NULL; };

    unsigned int nLights;
    DirectionalLight lights[MaxLights];
    std::vector<EclipseShadow>* shadows[MaxLights];

    Eigen::Vector3f eyeDir_obj;
    Eigen::Vector3f eyePos_obj;
    
    Eigen::Vector3f ambientColor;
};

#endif // _CELENGINE_LIGHTENV_H_
