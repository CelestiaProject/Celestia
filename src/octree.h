// octree.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _OCTREE_H_
#define _OCTREE_H_

#include <vector>
#include "quaternion.h"
#include "plane.h"
#include "star.h"


class StarHandler
{
 public:
    StarHandler() {};
    virtual ~StarHandler() {};
    
    virtual void process(const Star& star, float distance, float appMag) = 0;
};


class StarOctreeNode
{
 public:
    StarOctreeNode(const Point3f& _center, float _luminosity);
    ~StarOctreeNode();

    void insertStar(const Star&, float);
    void processVisibleStars(StarHandler& starHandler,
                             const Point3f& position,
                             const Planef* frustumPlanes,
                             float limitingMag,
                             float scale) const;
    int countChildren() const;
    int countStars() const;

 private:
    void addStar(const Star&);
    void split(float);
    void sortStarsIntoChildNodes();

    int nFaintStars;
    Point3f center;
    float luminosity;
    std::vector<const Star*>* stars;
    StarOctreeNode** children;
};


class StarOctree
{
 public:
    StarOctree(const Point3f& center, float _scale, float limitingMag);
    ~StarOctree();

    void insertStar(const Star&);
    void processVisibleStars(StarHandler& starHandler,
                             const Point3f& position,
                             const Quatf& orientation,
                             float fovY,
                             float aspectRatio,
                             float limitingMag) const;
    int countNodes() const;
    int countStars() const;

 private:
    StarOctreeNode* root;
    float scale;
    float luminosity;
};

#endif // _OCTREE_H_
