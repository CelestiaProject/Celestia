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

static const int splitThreshold = 100;

static const float sqrt3 = 1.732050808f;


StarOctree::StarOctree(const Point3f& center, float _scale, float limitingMag) :
    root(NULL), scale(_scale)
{
    luminosity = astro::appMagToLum(limitingMag, scale * sqrt3);
    root = new StarOctreeNode(center, luminosity);
}

StarOctree::~StarOctree()
{
    if (root != NULL)
        delete root;
}

void StarOctree::insertStar(const Star& star)
{
    root->insertStar(star, scale);
}

void StarOctree::processVisibleStars(StarHandler& starHandler,
                                     const Point3f& position,
                                     const Quatf& orientation,
                                     float fovY,
                                     float aspectRatio,
                                     float limitingMag) const
{
    // Compute the bounding planes of an infinite view frustum
    Planef frustumPlanes[5];
    Vec3f planeNormals[5];
    Mat3f rot = orientation.toMatrix3();
    float h = (float) tan(fovY / 2);
    float w = h * aspectRatio;
    planeNormals[0] = Vec3f(0, 1, -h);
    planeNormals[1] = Vec3f(0, -1, -h);
    planeNormals[2] = Vec3f(1, 0, -w);
    planeNormals[3] = Vec3f(-1, 0, -w);
    planeNormals[4] = Vec3f(0, 0, -1);
    for (int i = 0; i < 5; i++)
    {
        planeNormals[i].normalize();
        planeNormals[i] = planeNormals[i] * rot;
        frustumPlanes[i] = Planef(planeNormals[i], position);
    }
    
    root->processVisibleStars(starHandler, position, frustumPlanes,
                              limitingMag, scale);
}

int StarOctree::countNodes() const
{
    return 1 + root->countChildren();
}

int StarOctree::countStars() const
{
    return root->countStars();
}


StarOctreeNode::StarOctreeNode(const Point3f& _center, float _luminosity) :
    stars(NULL), children(NULL), center(_center), luminosity(_luminosity)
{
}

StarOctreeNode::~StarOctreeNode()
{
    if (children != NULL)
    {
        for (int i = 0; i < 8; i++)
        {
            delete children[i];
        }
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


void StarOctreeNode::insertStar(const Star& star, float scale)
{
    // If the star is brighter than the node's luminosity, insert
    // it here now.
    if (star.getLuminosity() >= luminosity)
    {
        addStar(star);
    }
    else
    {
        // If we haven't allocated child nodes yet, try to fit
        // the star in this node, even though it's fainter than
        // this node's luminosity.  Only if there are more than
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


void StarOctreeNode::processVisibleStars(StarHandler& starHandler,
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

    // Process the stars in this node
    if (stars != NULL)
    {
        for (vector<const Star*>::const_iterator iter = stars->begin();
             iter != stars->end(); iter++)
        {
            float distance = (position - (*iter)->getPosition()).length();
            float appMag = astro::lumToAppMag((*iter)->getLuminosity(), distance);
            if (appMag < limitingMag)
                starHandler.process(**iter, distance, appMag);
        }
    }

    // Compute the distance to node; this is equal to the distance to
    // the center of the node minus the radius of the node, scale * sqrt3.
    float distance = (position - center).length() - scale * sqrt3;
    if (distance <= 0 || astro::lumToAppMag(luminosity, distance) <= limitingMag)
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


void StarOctreeNode::addStar(const Star& star)
{
    if (stars == NULL)
        stars = new vector<const Star*>();
    stars->insert(stars->end(), &star);
}


void StarOctreeNode::split(float scale)
{
    children = new StarOctreeNode*[8];
    for (int i = 0; i < 8; i++)
    {
        Point3f p(center);
        p.x += ((i & XPos) != 0) ? scale : -scale;
        p.y += ((i & YPos) != 0) ? scale : -scale;
        p.z += ((i & ZPos) != 0) ? scale : -scale;
        children[i] = new StarOctreeNode(p, luminosity * 0.25f);
    }
    sortStarsIntoChildNodes();
}


// Sort this node's stars into stars that are bright enough to remain
// in the node, and stars that should be placed into one of the eight
// child nodes.
void StarOctreeNode::sortStarsIntoChildNodes()
{
    int nBrightStars = 0;

    for (int i = 0; i < stars->size(); i++)
    {
        const Star* star = (*stars)[i];
        if (star->getLuminosity() >= luminosity)
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


int StarOctreeNode::countChildren() const
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

int StarOctreeNode::countStars() const
{
    int nStars = stars == NULL ? 0 : stars->size();

    if (children != NULL)
    {
        for (int i = 0; i < 8; i++)
        {
            nStars += children[i]->countStars();
        }

    }

    return nStars;
}
