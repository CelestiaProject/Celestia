// objectrenderer.h
//
// Copyright (C) 2001-2019, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <Eigen/Core>
#include "octree.h"

class Observer;
class Renderer;

template <class OBJ, class PREC> class ObjectRenderer : public OctreeProcessor<OBJ, PREC>
{
 public:
    ObjectRenderer(PREC _distanceLimit) : distanceLimit((float) _distanceLimit) {};
    void process(const OBJ& /*unused*/, PREC /*unused*/, float /*unused*/) {};

    const Observer* observer    { nullptr };
    Renderer*  renderer         { nullptr };

    Eigen::Vector3f viewNormal;

    float fov                   { 0.0f };
    float size                  { 0.0f };
    float pixelSize             { 0.0f };
    float faintestMag           { 0.0f };
    float faintestMagNight      { 0.0f };
    float saturationMag         { 0.0f };
    float brightnessScale       { 0.0f };
    float brightnessBias        { 0.0f };
    float distanceLimit         { 0.0f };

    // Objects brighter than labelThresholdMag will be labeled
    float labelThresholdMag     { 0.0f };

    // These are not fully used by this template's descendants
    // but we place them here just in case a more sophisticated
    // rendering scheme is implemented:
    int nRendered               { 0 };
    int nClose                  { 0 };
    int nBright                 { 0 };
    int nProcessed              { 0 };
    int nLabelled               { 0 };

    uint64_t renderFlags        { 0 };
    int labelMode               { 0 };
};
