// octree.h
//
// Octree-based visibility determination for a star database.
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


class StarOctree;

class DynamicStarOctree
{
 public:
    DynamicStarOctree(const Point3f& _center, float _absMag);
    ~DynamicStarOctree();

    void insertStar(const Star&, float);
    void rebuildAndSort(StarOctree*& node, Star*& sortedStars);

 private:
    void addStar(const Star&);
    void split(float);
    void sortStarsIntoChildNodes();

    Point3f center;
    float absMag;
    std::vector<const Star*>* stars;
    DynamicStarOctree** children;
};


class StarOctree
{
 public:
    StarOctree(const Point3f& _center,
               float _absMag,
               Star* _firstStar,
               uint32 _nStars);
    ~StarOctree();

    void findVisibleStars(StarHandler& starHandler,
                          const Point3f& position,
                          const Planef* frustumPlanes,
                          float limitingMag,
                          float scale) const;
    void findCloseStars(StarHandler& starHandler,
                        const Point3f& position,
                        float radius,
                        float scale) const;
    int countChildren() const;
    int countStars() const;

    friend DynamicStarOctree;

 private:
    Point3f center;
    float absMag;
    Star* firstStar;
    uint32 nStars;
    StarOctree** children;
};


#endif // _OCTREE_H_
