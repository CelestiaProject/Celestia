// octree.cpp
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


enum
{
    XPos = 1,
    YPos = 2,
    ZPos = 4,
};

// The splitThreshold is the number of stars a node must contain before it's
// children are generated.  Increasing this number will decrease the number of
// octree nodes in the tree, which will use less memory but make culling less
// efficient.  In testing, changing splitThreshold from 100 to 50 nearly doubled
// the number of nodes in the tree, but provided anywhere from a 0 to 5 percent
// frame rate improvement.
static const int splitThreshold = 100;

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


void DynamicStarOctree::insertStar(const Star& star, float scale)
{
    // If the star is brighter than the node's magnitude, insert
    // it here now.
    if (star.getAbsoluteMagnitude() <= absMag)
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
            if (stars == NULL || stars->size() < splitThreshold)
            {
                // There's enough room left for the star in this node
                addStar(star);
            }
            else
            {
                // Not enough room in this node; we need to split it
                split(scale * 0.5f);
            }
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

    for (int i = 0; i < stars->size(); i++)
    {
        const Star* star = (*stars)[i];
        if (star->getAbsoluteMagnitude() <= absMag)
        {
            (*stars)[nBrightStars] = (*stars)[i];
            nBrightStars++;
        }
        else
        {
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


void StarOctree::processVisibleStars(StarHandler& starHandler,
                                     const Point3f& position,
                                     const Planef* frustumPlanes,
                                     float limitingMag,
                                     float scale) const
{
    // See if this node lies within the view frustum
    {
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
    float dimmest = minDistance > 0 ? astro::appToAbsMag(limitingMag, minDistance) : 100;
    for (int i = 0; i < nStars; i++)
    {
        Star& star = firstStar[i];
        if (star.getAbsoluteMagnitude() < dimmest)
        {
            float distance = (position - star.getPosition()).length();
            float appMag = astro::absToAppMag(star.getAbsoluteMagnitude(), distance);
            if (appMag < limitingMag)
                starHandler.process(star, distance, appMag);
        }
    }

    if (minDistance <= 0 || astro::absToAppMag(absMag, minDistance) <= limitingMag)
    {
        // Recurse into the child nodes
        if (children != NULL)
        {
            for (int i = 0; i < 8; i++)
            {
                children[i]->processVisibleStars(starHandler,
                                                 position,
                                                 frustumPlanes,
                                                 limitingMag,
                                                 scale * 0.5f);
            }
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
