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

    Eigen::Vector3d m_observerPos;
    std::array<Eigen::Hyperplane<PREC, 3>, 5> m_frustumPlanes;
    PREC m_distanceLimit;
    PREC m_distanceLimitSquared;
    float m_faintestMag;
    float m_absMagLimit{ std::numeric_limits<float>::max() };
};

template<typename PREC>
ObjectRenderer<PREC>::ObjectRenderer(const UniversalCoord& origin,
                                     const Eigen::Quaternionf& orientation,
                                     float fov,
                                     float aspectRatio,
                                     PREC distanceLimit,
                                     float faintestMag) :
    m_observerPos(origin.toLy()),
    m_distanceLimit(distanceLimit),
    m_distanceLimitSquared(celestia::math::square(distanceLimit)),
    m_faintestMag(faintestMag)
{
    using std::tan;

    PREC h = tan(celestia::math::degToRad(fov) * PREC(0.5));
    PREC w = h * static_cast<PREC>(aspectRatio);
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
        rot = orientation.conjugate();
    else
        rot = orientation.conjugate().cast<PREC>();

    Eigen::Matrix<PREC, 3, 1> observerPosPrec;
    if constexpr (std::is_same_v<PREC, double>)
        observerPosPrec = m_observerPos;
    else
        observerPosPrec = m_observerPos.cast<PREC>();

    for (unsigned int i = 0; i < 5; ++i)
    {
        m_frustumPlanes[i] = Eigen::Hyperplane<PREC, 3>(rot * planeNormals[i].normalized(), observerPosPrec);
    }
}

template<typename PREC>
bool
ObjectRenderer<PREC>::checkNode(const position_type& center,
                                PREC size,
                                float brightestMag)
{
    // Check if node intersects the view frustum
    for (const auto& plane : m_frustumPlanes)
    {
        PREC r = size * plane.normal().cwiseAbs().sum();
        if (plane.signedDistance(center) < -r)
            return false;
    }

    // Compute the distance to the node; this is equal to the distance to
    // the cellCenterPos of the node minus the boundingRadius of the node, scale * sqrt(3)
    PREC minDistance;
    if constexpr (std::is_same_v<std::remove_cv_t<PREC>, double>)
        minDistance = (m_observerPos - center).norm();
    else
        minDistance = (m_observerPos - center.template cast<double>()).norm();
    minDistance -= size * celestia::numbers::sqrt3_v<PREC>;
    if (minDistance > m_distanceLimit)
        return false;

    if (minDistance > 0 && celestia::astro::absToAppMag(static_cast<PREC>(brightestMag), minDistance) > m_faintestMag)
        return false;

    // To avoid unnecessary magnitude calculations, store the dimmest absolute magnitude
    // that we need to process
    m_absMagLimit = minDistance > 0
        ? static_cast<float>(celestia::astro::appToAbsMag(static_cast<PREC>(m_faintestMag), minDistance))
        : std::numeric_limits<float>::max();

    return true;
}
