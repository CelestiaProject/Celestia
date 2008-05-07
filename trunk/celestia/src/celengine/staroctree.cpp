//
// C++ Implementation: staroctree
//
// Description:
//
//
// Author: Toti <root@totibox>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <celengine/staroctree.h>


// Maximum permitted orbital radius for stars, in light years. Orbital
// radii larger than this value are not guaranteed to give correct
// results. The problem case is extremely faint stars (such as brown
// dwarfs.) The distance from the viewer to star's barycenter is used
// rough estimate of the brightness for the purpose of culling. When the
// star is very faint, this estimate may not work when the star is
// far from the barycenter. Thus, the star octree traversal will always
// render stars with orbits that are closer than MAX_STAR_ORBIT_RADIUS.
static const float MAX_STAR_ORBIT_RADIUS = 1.0f;


// The octree node into which a star is placed is dependent on two properties:
// its obsPosition and its luminosity--the fainter the star, the deeper the node
// in which it will reside.  Each node stores an absolute magnitude; no child
// of the node is allowed contain a star brighter than this value, making it
// possible to determine quickly whether or not to cull subtrees.

bool starAbsoluteMagnitudePredicate(const Star& star, const float absMag)
{
    return star.getAbsoluteMagnitude() <= absMag;
}


bool starOrbitStraddlesNodesPredicate(const Point3f& cellCenterPos, const Star& star, const float)
{
    //checks if this star's orbit straddles child nodes
    float orbitalRadius    = star.getOrbitalRadius();
    if (orbitalRadius == 0.0f)
        return false;

    Point3f starPos    = star.getPosition();

    return  abs(starPos.x - cellCenterPos.x) < orbitalRadius    ||
            abs(starPos.y - cellCenterPos.y) < orbitalRadius    ||
            abs(starPos.z - cellCenterPos.z) < orbitalRadius;
}


float starAbsoluteMagnitudeDecayFunction(const float excludingFactor)
{
    return astro::lumToAbsMag(astro::absMagToLum(excludingFactor) / 4.0f);
}


template<>
DynamicStarOctree* DynamicStarOctree::getChild(const Star&          obj,
                                               const Point3f& cellCenterPos)
{
    Point3f objPos    = obj.getPosition();

    int child = 0;
    child     |= objPos.x < cellCenterPos.x ? 0 : XPos;
    child     |= objPos.y < cellCenterPos.y ? 0 : YPos;
    child     |= objPos.z < cellCenterPos.z ? 0 : ZPos;

    return _children[child];
}


// In testing, changing SPLIT_THRESHOLD from 100 to 50 nearly
// doubled the number of nodes in the tree, but provided only between a
// 0 to 5 percent frame rate improvement.
template<> unsigned int DynamicStarOctree::SPLIT_THRESHOLD = 75;
template<> DynamicStarOctree::LimitingFactorPredicate*
           DynamicStarOctree::limitingFactorPredicate = starAbsoluteMagnitudePredicate;
template<> DynamicStarOctree::StraddlingPredicate*
           DynamicStarOctree::straddlingPredicate = starOrbitStraddlesNodesPredicate;
template<> DynamicStarOctree::ExclusionFactorDecayFunction*
           DynamicStarOctree::decayFunction = starAbsoluteMagnitudeDecayFunction;


// total specialization of the StaticOctree template process*() methods for stars:
template<>
void StarOctree::processVisibleObjects(StarHandler&   processor,
                                       const Point3f& obsPosition,
                                       const Planef*  frustumPlanes,
                                       float          limitingFactor,
                                       float          scale) const
{
    // See if this node lies within the view frustum

        // Test the cubic octree node against each one of the five
        // planes that define the infinite view frustum.
        for (int i=0; i<5; ++i)
        {
            const Planef* plane = frustumPlanes + i;
                  float   r     = scale * (abs(plane->normal.x) +
                                           abs(plane->normal.y) +
                                           abs(plane->normal.z));

            if (plane->normal * Vec3f(cellCenterPos.x, cellCenterPos.y, cellCenterPos.z) - plane->d < -r)
                return;
        }

    // Compute the distance to node; this is equal to the distance to
    // the cellCenterPos of the node minus the boundingRadius of the node, scale * SQRT3.
    float minDistance = (obsPosition - cellCenterPos).length() - scale * StarOctree::SQRT3;

    // Process the objects in this node
    float dimmest     = minDistance > 0 ? astro::appToAbsMag(limitingFactor, minDistance) : 1000;

    for (unsigned int i=0; i<nObjects; ++i)
    {
        Star& obj = _firstObject[i];

        if (obj.getAbsoluteMagnitude() < dimmest)
        {
            float distance    = obsPosition.distanceTo(obj.getPosition());
            float appMag      = astro::absToAppMag(obj.getAbsoluteMagnitude(), distance);
            
            if (appMag < limitingFactor || (distance < MAX_STAR_ORBIT_RADIUS && obj.getOrbit()))
                processor.process(obj, distance, appMag);
        }
    }

    // See if any of the objects in child nodes are potentially included
    // that we need to recurse deeper.
    if (minDistance <= 0 || astro::absToAppMag(exclusionFactor, minDistance) <= limitingFactor)
    {
        // Recurse into the child nodes
        if (_children != NULL)
        {
            for (int i=0; i<8; ++i)
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
void StarOctree::processCloseObjects(StarHandler&   processor,
                                     const Point3f& obsPosition,
                                     float          boundingRadius,
                                     float          scale) const
{
    // Compute the distance to node; this is equal to the distance to
    // the cellCenterPos of the node minus the boundingRadius of the node, scale * SQRT3.
    float nodeDistance    = (obsPosition - cellCenterPos).length() - scale * StarOctree::SQRT3;

    if (nodeDistance > boundingRadius)
        return;

    // At this point, we've determined that the cellCenterPos of the node is
    // close enough that we must check individual objects for proximity.

    // Compute distance squared to avoid having to sqrt for distance
    // comparison.
    float radiusSquared    = boundingRadius * boundingRadius;

    // Check all the objects in the node.
    for (unsigned int i = 0; i < nObjects; ++i)
    {
        Star& obj = _firstObject[i];

        if (obsPosition.distanceToSquared(obj.getPosition()) < radiusSquared)
        {
            float distance    = obsPosition.distanceTo(obj.getPosition());
            float appMag      = astro::absToAppMag(obj.getAbsoluteMagnitude(), distance);

            processor.process(obj, distance, appMag);
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
