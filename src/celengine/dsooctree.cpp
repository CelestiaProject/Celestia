// dsooctree.cpp
//
// Description:
//
// Copyright (C) 2005-2009, Celestia Development Team
// Original version by Toti <root@totibox>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "dsooctree.h"

#include <celastro/astro.h>
#include <celcompat/numbers.h>
#include <celmath/mathlib.h>

namespace celestia::engine
{

// The version of cppcheck used by Codacy doesn't seem to detect the field initializer

DSOOctreeVisibleObjectsProcessor::DSOOctreeVisibleObjectsProcessor(DSOHandler* dsoHandler, // cppcheck-suppress uninitMemberVar
                                                                   const DSOOctree::PointType& obsPosition,
                                                                   util::array_view<PlaneType> frustumPlanes,
                                                                   float limitingFactor) :
    m_dsoHandler(dsoHandler),
    m_obsPosition(obsPosition),
    m_frustumPlanes(frustumPlanes),
    m_limitingFactor(limitingFactor)
{
}

bool
DSOOctreeVisibleObjectsProcessor::checkNode(const DSOOctree::PointType& center,
                                            double size,
                                            float factor)
{
    // Test the cubic octree node against each one of the five
    // planes that define the infinite view frustum.
    for (unsigned int i = 0; i < 5; ++i)
    {
        const PlaneType& plane = m_frustumPlanes[i];

        double r = size * plane.normal().cwiseAbs().sum();
        if (plane.signedDistance(center) < -r)
            return false;
    }

    // Compute the distance to node; this is equal to the distance to
    // the cellCenterPos of the node minus the boundingRadius of the node, scale * SQRT3.
    double minDistance = (m_obsPosition - center).norm() - size * numbers::sqrt3;

    // Check whether the brightest object in this node is bright enough to render
    auto distanceModulus = static_cast<float>(astro::distanceModulus(minDistance));
    if (minDistance > 0.0 && (factor + distanceModulus) > m_limitingFactor)
        return false;

    // Dimmest absolute magnitude to process
    m_dimmest = minDistance > 0.0 ? (m_limitingFactor - distanceModulus) : 1000.0;

    return true;
}

void
DSOOctreeVisibleObjectsProcessor::process(const std::unique_ptr<DeepSkyObject>& obj) const //NOSONAR
{
    float absMag = obj->getAbsoluteMagnitude();
    if (absMag > m_dimmest)
        return;

    double distance = (m_obsPosition - obj->getPosition()).norm() - obj->getBoundingSphereRadius();
    auto appMag = static_cast<float>((distance >= 32.6167) ? astro::absToAppMag(static_cast<double>(absMag), distance) : absMag);

    if (appMag <= m_limitingFactor)
        m_dsoHandler->process(obj, distance, absMag);
}

DSOOctreeCloseObjectsProcessor::DSOOctreeCloseObjectsProcessor(DSOHandler* dsoHandler,
                                                               const DSOOctree::PointType& obsPosition,
                                                               double boundingRadius) :
    m_dsoHandler(dsoHandler),
    m_obsPosition(obsPosition),
    m_boundingRadius(boundingRadius),
    m_radiusSquared(math::square(boundingRadius))
{
}

bool
DSOOctreeCloseObjectsProcessor::checkNode(const DSOOctree::PointType& center,
                                          double size,
                                          float /* factor */) const
{
    // Compute the distance to node; this is equal to the distance to
    // the cellCenterPos of the node minus the boundingRadius of the node, scale * SQRT3.
    double nodeDistance = (m_obsPosition - center).norm() - size * numbers::sqrt3;
    return nodeDistance <= m_boundingRadius;
}

void
DSOOctreeCloseObjectsProcessor::process(const std::unique_ptr<DeepSkyObject>& obj) const //NOSONAR
{
    Eigen::Vector3d offset = m_obsPosition - obj->getPosition();
    if (offset.squaredNorm() < m_radiusSquared)
    {
        float  absMag   = obj->getAbsoluteMagnitude();
        double distance = offset.norm() - obj->getBoundingSphereRadius();
        m_dsoHandler->process(obj, distance, absMag);
    }
}

} // end namespace celestia::engine
