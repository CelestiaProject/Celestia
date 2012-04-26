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

#include <celengine/dsooctree.h>
#include <Eigen/Array>

using namespace Eigen;


// The octree node into which a dso is placed is dependent on two properties:
// its obsPosition and its luminosity--the fainter the dso, the deeper the node
// in which it will reside.  Each node stores an absolute magnitude; no child
// of the node is allowed contain a dso brighter than this value, making it
// possible to determine quickly whether or not to cull subtrees.

bool dsoAbsoluteMagnitudePredicate(DeepSkyObject* const & _dso, const float absMag)
{
    return _dso->getAbsoluteMagnitude() <= absMag;
}


bool dsoStraddlesNodesPredicate(const Vector3d& cellCenterPos, DeepSkyObject* const & _dso, const float)
{
    //checks if this dso's radius straddles child nodes
    float dsoRadius    = _dso->getBoundingSphereRadius();

    return (_dso->getPosition() - cellCenterPos).cwise().abs().minCoeff() < dsoRadius;
#ifdef CELVEC
    return abs(dsoPos.x - cellCenterPos.x) < dsoRadius    ||
           abs(dsoPos.y - cellCenterPos.y) < dsoRadius    ||
           abs(dsoPos.z - cellCenterPos.z) < dsoRadius;
#endif
}


double dsoAbsoluteMagnitudeDecayFunction(const double excludingFactor)
{
    return excludingFactor + 0.5f;
}


template <>
DynamicDSOOctree* DynamicDSOOctree::getChild(DeepSkyObject* const & _obj, const PointType& cellCenterPos)
{
    PointType objPos = _obj->getPosition();

    int child = 0;
    child     |= objPos.x() < cellCenterPos.x() ? 0 : XPos;
    child     |= objPos.y() < cellCenterPos.y() ? 0 : YPos;
    child     |= objPos.z() < cellCenterPos.z() ? 0 : ZPos;

    return _children[child];
}


template<> unsigned int DynamicDSOOctree::SPLIT_THRESHOLD = 10;
template<> DynamicDSOOctree::LimitingFactorPredicate*
           DynamicDSOOctree::limitingFactorPredicate = dsoAbsoluteMagnitudePredicate;
template<> DynamicDSOOctree::StraddlingPredicate*
           DynamicDSOOctree::straddlingPredicate = dsoStraddlesNodesPredicate;
template<> DynamicDSOOctree::ExclusionFactorDecayFunction*
           DynamicDSOOctree::decayFunction = dsoAbsoluteMagnitudeDecayFunction;


// total specialization of the StaticOctree template process*() methods for DSOs:
template<>
void DSOOctree::processVisibleObjects(DSOHandler&    processor,
                                      const PointType& obsPosition,
                                      const Hyperplane<double, 3>*  frustumPlanes,
                                      float          limitingFactor,
                                      double         scale) const
{
    // See if this node lies within the view frustum

    // Test the cubic octree node against each one of the five
    // planes that define the infinite view frustum.
    for (unsigned int i = 0; i < 5; ++i)
    {
        const Hyperplane<double, 3>& plane = frustumPlanes[i];

        double r = scale * plane.normal().cwise().abs().sum();
        if (plane.signedDistance(cellCenterPos) < -r)
            return;

#ifdef CELVEC
        double  r     = scale * (abs(plane->normal.x) +
                                 abs(plane->normal.y) +
                                 abs(plane->normal.z));

        if (plane->normal * cellCenterPos - plane->d < -r)
            return;
#endif
    }

    // Compute the distance to node; this is equal to the distance to
    // the cellCenterPos of the node minus the boundingRadius of the node, scale * SQRT3.
    double minDistance = (obsPosition - cellCenterPos).norm() - scale * DSOOctree::SQRT3;

    // Process the objects in this node
    double dimmest     = minDistance > 0.0 ? astro::appToAbsMag((double) limitingFactor, minDistance) : 1000.0;

    for (unsigned int i=0; i<nObjects; ++i)
    {
        DeepSkyObject* _obj = _firstObject[i];
        float  absMag      = _obj->getAbsoluteMagnitude();
        if (absMag < dimmest)
        {
            double distance    = (obsPosition - _obj->getPosition()).norm() - _obj->getBoundingSphereRadius();
            float appMag = (float) ((distance >= 32.6167) ? astro::absToAppMag((double) absMag, distance) : absMag);

            if ( appMag < limitingFactor)
                processor.process(_obj, distance, absMag);
        }
    }

    // See if any of the objects in child nodes are potentially included
    // that we need to recurse deeper.
    if (minDistance <= 0.0 || astro::absToAppMag((double) exclusionFactor, minDistance) <= limitingFactor)
    {
        // Recurse into the child nodes
        if (_children != NULL)
        {
            for (int i = 0; i < 8; ++i)
            {
                _children[i]->processVisibleObjects(processor,
                                                    obsPosition,
                                                    frustumPlanes,
                                                    limitingFactor,
                                                    scale * 0.5f);
            }
        }
    }
}


template<>
void DSOOctree::processCloseObjects(DSOHandler&    processor,
                                    const PointType& obsPosition,
                                    double         boundingRadius,
                                    double         scale) const
{
    // Compute the distance to node; this is equal to the distance to
    // the cellCenterPos of the node minus the boundingRadius of the node, scale * SQRT3.
    double nodeDistance    = (obsPosition - cellCenterPos).norm() - scale * DSOOctree::SQRT3;    //

    if (nodeDistance > boundingRadius)
        return;

    // At this point, we've determined that the cellCenterPos of the node is
    // close enough that we must check individual objects for proximity.

    // Compute distance squared to avoid having to sqrt for distance
    // comparison.
    double radiusSquared    = boundingRadius * boundingRadius;    //

    // Check all the objects in the node.
    for (unsigned int i=0; i<nObjects; ++i)
    {
        DeepSkyObject* _obj = _firstObject[i];        //

        if ((obsPosition - _obj->getPosition()).squaredNorm() < radiusSquared)    //
        {
            float  absMag      = _obj->getAbsoluteMagnitude();
            double distance    = (obsPosition - _obj->getPosition()).norm() - _obj->getBoundingSphereRadius();

            processor.process(_obj, distance, absMag);
        }
    }

    // Recurse into the child nodes
    if (_children != NULL)
    {
        for (int i = 0; i < 8; ++i)
        {
            _children[i]->processCloseObjects(processor,
                                              obsPosition,
                                              boundingRadius,
                                              scale * 0.5f);
        }
    }
}
