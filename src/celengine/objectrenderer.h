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

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <celastro/astro.h>
#include <celcompat/numbers.h>
#include <celmath/mathlib.h>
#include "octree.h"
#include "univcoord.h"

template<typename PREC>
class ObjectRenderer
{
public:
    using position_type = Eigen::Matrix<PREC, 3, 1>;

    bool checkNode(const position_type&, PREC, float);

protected:
    ObjectRenderer(const UniversalCoord&, const Eigen::Quaternionf&, float, float, PREC, float);
    ~ObjectRenderer() = default;

    position_type getRelativePosition(const position_type& position) const;

    Eigen::Vector3d observerPos;
    std::array<Eigen::Hyperplane<PREC, 3>, 5> frustumPlanes;
    PREC distanceLimit;
    PREC distanceLimitSquared;
    float faintestMag;
    float absMagLimit{ std::numeric_limits<float>::max() };
};

template<typename PREC>
ObjectRenderer<PREC>::ObjectRenderer(const UniversalCoord& _origin,
                                     const Eigen::Quaternionf& _orientation,
                                     float _fov,
                                     float _aspectRatio,
                                     PREC _distanceLimit,
                                     float _faintestMag) :
    observerPos(_origin.toLy()),
    distanceLimit(_distanceLimit),
    distanceLimitSquared(_distanceLimit * _distanceLimit),
    faintestMag(_faintestMag)
{
    using std::tan;

    PREC h = tan(celestia::math::degToRad(_fov) * PREC(0.5));
    PREC w = h * static_cast<PREC>(_aspectRatio);
    std::array<position_type, 5> planeNormals
    {
        position_type( 0,  1, -h),
        position_type( 0, -1, -h),
        position_type( 1,  0, -w),
        position_type(-1,  0, -w),
        position_type( 0,  0, -1),
    };

    Eigen::Quaternion<PREC> rot;
    if constexpr (std::is_same_v<PREC, float>)
        rot = _orientation.conjugate();
    else
        rot = _orientation.conjugate().cast<PREC>();

    Eigen::Matrix<PREC, 3, 1> observerPosPrec;
    if constexpr (std::is_same_v<PREC, double>)
        observerPosPrec = observerPos;
    else
        observerPosPrec = observerPos.cast<PREC>();

    for (unsigned int i = 0; i < 5; ++i)
    {
        frustumPlanes[i] = Eigen::Hyperplane<PREC, 3>(rot * planeNormals[i].normalized(), observerPosPrec);
    }
}

template<typename PREC>
bool
ObjectRenderer<PREC>::checkNode(const position_type& center,
                                PREC size,
                                float brightestMag)
{
    // Check if node intersects the view frustum
    for (const auto& plane : frustumPlanes)
    {
        PREC r = size * plane.normal().cwiseAbs().sum();
        if (plane.signedDistance(center) < -r)
            return false;
    }

    // Compute the distance to the node; this is equal to the distance to
    // the cellCenterPos of the node minus the boundingRadius of the node, scale * sqrt(3)
    PREC minDistance = (observerPos - center).norm() - size * celestia::numbers::sqrt3_v<PREC>;
    if (minDistance > distanceLimit)
        return false;

    if (minDistance > 0 && celestia::astro::absToAppMag(static_cast<PREC>(brightestMag), minDistance) > faintestMag)
        return false;

    // To avoid unnecessary magnitude calculations, store the dimmest absolute magnitude
    // that we need to process
    absMagLimit = minDistance > 0
        ? static_cast<float>(celestia::astro::appToAbsMag(static_cast<PREC>(faintestMag), minDistance))
        : std::numeric_limits<float>::max();

    return true;
}
