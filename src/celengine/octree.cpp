// octree.cpp
//
// Octree-based visibility determination for a star database.
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cmath>
#include "astro.h"
#include "octree.h"

using namespace std;


// There are two classed implemented in this module: StarOctree and
// DynamicStarOctree.  The DynamicStarOctree is built first by inserting
// stars from a database and is then 'compiled' into a StarOctree.  In the
// process of building the StarOctree, the original star database is
// reorganized, with stars in the same octree node all placed adjacent to each
// other.  This spatial sorting of the stars dramatically improves the
// performance of octree operations through much more coherent memory access.
//
// The octree node into which a star is placed is dependent on two properties:
// its position and its luminosity--the fainter the star, the deeper the node
// in which it will reside.  Each node stores an absolute magnitude; no child
// of the node is allowed contain star brighter than this value, making it
// possible to determine quickly whether or not to cull subtrees.

enum
{
    XPos = 1,
    YPos = 2,
    ZPos = 4,
};

// The splitThreshold is the number of stars a node must contain before it's
// children are generated.  Increasing this number will decrease the number of
// octree nodes in the tree, which will use less memory but make culling less
// efficient.  In testing, changing splitThreshold from 100 to 50 nearly
// doubled the number of nodes in the tree, but provided only between a
// 0 to 5 percent frame rate improvement.
static const unsigned int splitThreshold = 100;

static const float sqrt3 = 1.732050808f;


DynamicStarOctree::DynamicStarOctree(const Point3f& _center, float _absMag) :
    stars(NULL), children(NULL), center(_center), absMag(_absMag)
{
}

DynamicStarOctree::~DynamicStarOctree()
{
    if (children != NULL)
    {
        for (int i = 0; i < 8; i++)
        {
            delete children[i];
        }
        delete [] children;
    }
}


static int childIndex(const Star& star, const Point3f& center)
{
    Point3f pos = star.getPosition();

    int child = 0;
    child |= pos.x < center.x ? 0 : XPos;
    child |= pos.y < center.y ? 0 : YPos;
    child |= pos.z < center.z ? 0 : ZPos;
    
    return child;
}


inline bool
starOrbitStraddlesNodes(const Star& star, const Point3f& center)
{
    float orbitalRadius = star.getOrbitalRadius();
    if (orbitalRadius == 0.0f)
        return false;

    Point3f starPos = star.getPosition();
    return (abs(starPos.x - center.x) < orbitalRadius ||
            abs(starPos.y - center.y) < orbitalRadius ||
            abs(starPos.z - center.z) < orbitalRadius);
}


void DynamicStarOctree::insertStar(const Star& star, float scale)
{
    // If the star is brighter than the node's magnitude, insert
    // it here now.  Also, if the star is in an orbit in a multiple
    // star system, check for the case where the sphere containing
    // the orbit straddles two or more child nodes.  If it does, we
    // must keep the star in the parent.
    if (star.getAbsoluteMagnitude() <= absMag ||
        starOrbitStraddlesNodes(star, center))
    {
        addStar(star);
    }
    else
    {
        // If we haven't allocated child nodes yet, try to fit
        // the star in this node, even though it's fainter than
        // this node's magnitude.  Only if there are more than
        // splitThreshold stars in the node will we attempt to
        // place the star into a child node.  This is done in order
        // to avoid having the octree degenerate into one star per node.
        if (children == NULL)
        {
            // Make sure that there's enough room left in this node
            if (stars != NULL && stars->size() >= splitThreshold)
                split(scale * 0.5f);
            addStar(star);
        }
        else
        {
            // We've already allocated child nodes; place the star
            // into the appropriate one.
            children[childIndex(star, center)]->insertStar(star, scale * 0.5f);
        }
    }
}


void DynamicStarOctree::addStar(const Star& star)
{
    if (stars == NULL)
        stars = new vector<const Star*>();
    stars->insert(stars->end(), &star);
}


void DynamicStarOctree::split(float scale)
{
    children = new DynamicStarOctree*[8];
    for (int i = 0; i < 8; i++)
    {
        Point3f p(center);
        p.x += ((i & XPos) != 0) ? scale : -scale;
        p.y += ((i & YPos) != 0) ? scale : -scale;
        p.z += ((i & ZPos) != 0) ? scale : -scale;
        children[i] = new DynamicStarOctree(p, astro::lumToAbsMag(astro::absMagToLum(absMag) / 4.0f));
    }
    sortStarsIntoChildNodes();
}


// Sort this node's stars into stars that are bright enough to remain
// in the node, and stars that should be placed into one of the eight
// child nodes.
void DynamicStarOctree::sortStarsIntoChildNodes()
{
    int nBrightStars = 0;

    for (unsigned int i = 0; i < stars->size(); i++)
    {
        const Star* star = (*stars)[i];

        if (star->getAbsoluteMagnitude() <= absMag ||
            starOrbitStraddlesNodes(*star, center))
        {
            // If the star is bright enough or if it's in an orbit in
            // a multiple star system with an orbit straddling two or
            // more child nodes, then keep the star in the parent...
            (*stars)[nBrightStars] = (*stars)[i];
            nBrightStars++;
        }
        else
        {
            // ...otherwise, assign it to a child node.
            children[childIndex(*star, center)]->addStar(*star);
        }
    }

    stars->resize(nBrightStars);
}


void DynamicStarOctree::rebuildAndSort(StarOctree*& node, Star*& sortedStars)
{
    Star* firstStar = sortedStars;

    if (stars != NULL)
    {
        for (vector<const Star*>::const_iterator iter = stars->begin();
             iter != stars->end(); iter++)
        {
            *sortedStars++ = **iter;
        }
    }

    uint32 nStars = (uint32) (sortedStars - firstStar);
    node = new StarOctree(center, absMag, firstStar, nStars);

    if (children != NULL)
    {
        node->children = new StarOctree*[8];
        for (int i = 0; i < 8; i++)
            children[i]->rebuildAndSort(node->children[i], sortedStars);
    }
}



StarOctree::StarOctree(const Point3f& _center,
                       float _absMag,
                       Star* _firstStar,
                       uint32 _nStars) :
    center(_center),
    absMag(_absMag),
    firstStar(_firstStar),
    nStars(_nStars),
    children(NULL)
{
}

StarOctree::~StarOctree()
{
    if (children != NULL)
    {
        for (int i = 0; i < 8; i++)
            delete children[i];
        delete [] children;
    }
}


// This method searches the octree for stars that are likely to be visible
// to a viewer with the specified position and limiting magnitude.  The
// starHandler is invoked for each potentially visible star--no star with
// a magnitude greater than the limiting magnitude will be processed, but
// stars that are outside the view frustum may be.  Frustum tests are performed
// only at the node level to optimize the octree traversal, so an exact test
// (if one is required) is the responsibility of the callback method.
void StarOctree::findVisibleStars(StarHandler& starHandler,
                                  const Point3f& position,
                                  const Planef* frustumPlanes,
                                  float limitingMag,
                                  float scale) const
{
    // See if this node lies within the view frustum
    {
        // Test the cubic octree node against each one of the five
        // planes that define the infinite view frustum.
        for (int i = 0; i < 5; i++)
        {
            const Planef* plane = frustumPlanes + i;
            float r = scale * (abs(plane->normal.x) +
                               abs(plane->normal.y) +
                               abs(plane->normal.z));
            if (plane->normal * Vec3f(center.x, center.y, center.z) - plane->d < -r)
                return;
        }
    }

    // Compute the distance to node; this is equal to the distance to
    // the center of the node minus the radius of the node, scale * sqrt3.
    float minDistance = (position - center).length() - scale * sqrt3;

    // Process the stars in this node
    float dimmest = minDistance > 0 ? astro::appToAbsMag(limitingMag, minDistance) : 1000;
    for (unsigned int i = 0; i < nStars; i++)
    {
        Star& star = firstStar[i];
        if (star.getAbsoluteMagnitude() < dimmest)
        {
            float distance = position.distanceTo(star.getPosition());
            float appMag = astro::absToAppMag(star.getAbsoluteMagnitude(), distance);
            if (appMag < limitingMag)
                starHandler.process(star, distance, appMag);
        }
    }

    // See if any of the stars in child nodes are potentially bright enough
    // that we need to recurse deeper.
    if (minDistance <= 0 || astro::absToAppMag(absMag, minDistance) <= limitingMag)
    {
        // Recurse into the child nodes
        if (children != NULL)
        {
            for (int i = 0; i < 8; i++)
            {
                children[i]->findVisibleStars(starHandler,
                                              position,
                                              frustumPlanes,
                                              limitingMag,
                                              scale * 0.5f);
            }
        }
    }
}


void StarOctree::findCloseStars(StarHandler& starHandler,
                                const Point3f& position,
                                float radius,
                                float scale) const
{
    // Compute the distance to node; this is equal to the distance to
    // the center of the node minus the radius of the node, scale * sqrt3.
    float nodeDistance = (position - center).length() - scale * sqrt3;
    if (nodeDistance > radius)
        return;

    // At this point, we've determined that the center of the node is
    // close enough that we must check individual stars for proximity.

    // Compute distance squared to avoid having to sqrt for distance
    // comparison.
    float radiusSquared = radius * radius;

    // Check all the stars in the node.
    for (unsigned int i = 0; i < nStars; i++)
    {
        Star& star = firstStar[i];
        if (position.distanceToSquared(star.getPosition()) < radiusSquared)
        {
            float distance = position.distanceTo(star.getPosition());
            float appMag = astro::absToAppMag(star.getAbsoluteMagnitude(),
                                              distance);
            starHandler.process(star, distance, appMag);
        }
    }

    // Recurse into the child nodes
    if (children != NULL)
    {
        for (int i = 0; i < 8; i++)
        {
            children[i]->findCloseStars(starHandler,
                                        position,
                                        radius,
                                        scale * 0.5f);
        }
    }
}



int StarOctree::countChildren() const
{
    if (children == NULL)
    {
        return 0;
    }
    else
    {
        int nChildren = 0;
        for (int i = 0; i < 8; i++)
            nChildren += 1 + children[i]->countChildren();

        return nChildren;
    }
}

int StarOctree::countStars() const
{
    int count = nStars;

    if (children != NULL)
    {
        for (int i = 0; i < 8; i++)
            count += children[i]->countStars();
    }

    return count;
}
