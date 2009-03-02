//
// C++ Implementation: dsooctree
//
// Description:
//
//
// Author: Toti <root@totibox>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <celengine/dsooctree.h>

// The octree node into which a dso is placed is dependent on two properties:
// its obsPosition and its luminosity--the fainter the dso, the deeper the node
// in which it will reside.  Each node stores an absolute magnitude; no child
// of the node is allowed contain a dso brighter than this value, making it
// possible to determine quickly whether or not to cull subtrees.

bool dsoAbsoluteMagnitudePredicate(DeepSkyObject* const & _dso, const float absMag)
{
    return _dso->getAbsoluteMagnitude() <= absMag;
}


bool dsoStraddlesNodesPredicate(const Point3d& cellCenterPos, DeepSkyObject* const & _dso, const float)
{
    //checks if this dso's radius straddles child nodes
    float dsoRadius    = _dso->getBoundingSphereRadius();

    Point3d dsoPos     = _dso->getPosition();

    return abs(dsoPos.x - cellCenterPos.x) < dsoRadius    ||
           abs(dsoPos.y - cellCenterPos.y) < dsoRadius    ||
           abs(dsoPos.z - cellCenterPos.z) < dsoRadius;
}


double dsoAbsoluteMagnitudeDecayFunction(const double excludingFactor)
{
    return excludingFactor + 0.5f;
}


template <>
DynamicDSOOctree* DynamicDSOOctree::getChild(DeepSkyObject* const & _obj, const Point3d& cellCenterPos)
{
    Point3d objPos    = _obj->getPosition();

    int child = 0;
    child     |= objPos.x < cellCenterPos.x ? 0 : XPos;
    child     |= objPos.y < cellCenterPos.y ? 0 : YPos;
    child     |= objPos.z < cellCenterPos.z ? 0 : ZPos;

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
                                      const Point3d& obsPosition,
                                      const Planed*  frustumPlanes,
                                      float          limitingFactor,
                                      double         scale) const
{
    // See if this node lies within the view frustum

    // Test the cubic octree node against each one of the five
    // planes that define the infinite view frustum.
    for (int i = 0; i < 5; ++i)
    {
        const Planed* plane = frustumPlanes + i;
        double  r     = scale * (abs(plane->normal.x) +
                                 abs(plane->normal.y) +
                                 abs(plane->normal.z));

        if (plane->normal * Vec3d(cellCenterPos.x, cellCenterPos.y, cellCenterPos.z) - plane->d < -r)
            return;
    }

    // Compute the distance to node; this is equal to the distance to
    // the cellCenterPos of the node minus the boundingRadius of the node, scale * SQRT3.
    double minDistance = (obsPosition - cellCenterPos).length() - scale * DSOOctree::SQRT3;

    // Process the objects in this node
    double dimmest     = minDistance > 0.0 ? astro::appToAbsMag((double) limitingFactor, minDistance) : 1000.0;

    for (unsigned int i=0; i<nObjects; ++i)
    {
        DeepSkyObject* _obj = _firstObject[i];
        float  absMag      = _obj->getAbsoluteMagnitude();
        if (absMag < dimmest)
        {
            double distance    = obsPosition.distanceTo(_obj->getPosition()) - _obj->getBoundingSphereRadius();
            float appMag = (float) ((distance >= 32.6167) ? astro::absToAppMag((double) absMag, distance) : absMag);

            if ( appMag < limitingFactor)
                processor.process(_obj, distance, absMag);
        }
    }

    // See if any of the objects in child nodes are potentially included
    // that we need to recurse deeper.
    if (minDistance <= 0.0 || astro::absToAppMag((double) exclusionFactor, minDistance) <= limitingFactor)
        // Recurse into the child nodes
        if (_children != NULL)
            for (int i=0; i<8; ++i)
            {
                _children[i]->processVisibleObjects(processor,
                                                    obsPosition,
                                                    frustumPlanes,
                                                    limitingFactor,
                                                    scale * 0.5f);
            }
}


template<>
void DSOOctree::processCloseObjects(DSOHandler&    processor,
                                    const Point3d& obsPosition,
                                    double         boundingRadius,
                                    double         scale) const
{
    // Compute the distance to node; this is equal to the distance to
    // the cellCenterPos of the node minus the boundingRadius of the node, scale * SQRT3.
    double nodeDistance    = (obsPosition - cellCenterPos).length() - scale * DSOOctree::SQRT3;    //

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

        if (obsPosition.distanceToSquared(_obj->getPosition()) < radiusSquared)    //
        {
            float  absMag      = _obj->getAbsoluteMagnitude();
            double distance    = obsPosition.distanceTo(_obj->getPosition()) - _obj->getBoundingSphereRadius();

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
