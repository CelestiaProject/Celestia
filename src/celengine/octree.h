// octree.h
//
// Octree-based visibility determination for objects.
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
#include <celmath/quaternion.h>
#include <celmath/plane.h>
#include <celengine/observer.h>


// The DynamicOctree and StaticOctree template arguments are:
// OBJ:  object hanging from the node,
// PREC: floating point precision of the culling operations at node level.
// The hierarchy of octree nodes is built using a single precision value (excludingFactor), which relates to an
// OBJ's limiting property defined by the octree particular specialization: ie. we use [absolute magnitude] for star octrees, etc.
// For details, see notes below.

template <class OBJ, class PREC> class OctreeProcessor
{
 public:
    OctreeProcessor()          {};
    virtual ~OctreeProcessor() {};

    virtual void process(const OBJ& obj, PREC distance, float appMag) = 0;
};



struct OctreeLevelStatistics
{
    unsigned int nodeCount;
    unsigned int objectCount;
    double size;
};


template <class OBJ, class PREC> class StaticOctree;
template <class OBJ, class PREC> class DynamicOctree
{
 typedef std::vector<const OBJ*> ObjectList;

 typedef bool (LimitingFactorPredicate)     (const OBJ&, const float);
 typedef bool (StraddlingPredicate)         (const Point3<PREC>&, const OBJ&, const float);
 typedef PREC (ExclusionFactorDecayFunction)(const PREC);

 public:
    DynamicOctree(const Point3<PREC>& cellCenterPos,
                  const float         exclusionFactor);
    ~DynamicOctree();

    void insertObject  (const OBJ&, const PREC);
    void rebuildAndSort(StaticOctree<OBJ, PREC>*&, OBJ*&);

 private:
   static unsigned int SPLIT_THRESHOLD;

   static LimitingFactorPredicate*      limitingFactorPredicate;
   static StraddlingPredicate*          straddlingPredicate;
   static ExclusionFactorDecayFunction* decayFunction;

 private:
    void           add  (const OBJ&);
    void           split(const PREC);
    void           sortIntoChildNodes();
    DynamicOctree* getChild(const OBJ&, const Point3<PREC>&);

    DynamicOctree** _children;
    Point3<PREC>    cellCenterPos;
    PREC            exclusionFactor;
    ObjectList*     _objects;
};





template <class OBJ, class PREC> class StaticOctree
{
 friend class DynamicOctree<OBJ, PREC>;

 public:
    StaticOctree(const Point3<PREC>& cellCenterPos,
                 const float         exclusionFactor,
                 OBJ*                _firstObject,
                 unsigned int        nObjects);
    ~StaticOctree();

    // These methods are only declared at the template level; we'll implement them as
    // full specializations, allowing for different traversal strategies depending on the
    // object type and nature.

    // This method searches the octree for objects that are likely to be visible
    // to a viewer with the specified obsPosition and limitingFactor.  The
    // octreeProcessor is invoked for each potentially visible object --no object with
    // a property greater than limitingFactor will be processed, but
    // objects that are outside the view frustum may be.  Frustum tests are performed
    // only at the node level to optimize the octree traversal, so an exact test
    // (if one is required) is the responsibility of the callback method.
    void processVisibleObjects(OctreeProcessor<OBJ, PREC>& processor,
                               const Point3<PREC>&         obsPosition,
                               const Plane<PREC>*          frustumPlanes,
                               float                       limitingFactor,
                               PREC                        scale) const;

    void processCloseObjects(OctreeProcessor<OBJ, PREC>& processor,
                             const Point3<PREC>&         obsPosition,
                             PREC                        boundingRadius,
                             PREC                        scale) const;

    int countChildren() const;
    int countObjects()  const;

    void computeStatistics(std::vector<OctreeLevelStatistics>& stats, unsigned int level = 0);

 private:
    static const PREC SQRT3;

 private:
    StaticOctree** _children;
    Point3<PREC>   cellCenterPos;
    float          exclusionFactor;
    OBJ*           _firstObject;
    unsigned int   nObjects;
};








// There are two classes implemented in this module: StaticOctree and
// DynamicOctree.  The DynamicOctree is built first by inserting
// objects from a database or catalog and is then 'compiled' into a StaticOctree.
// In the process of building the StaticOctree, the original object database is
// reorganized, with objects in the same octree node all placed adjacent to each
// other.  This spatial sorting of the objects dramatically improves the
// performance of octree operations through much more coherent memory access.
enum
{
    XPos = 1,
    YPos = 2,
    ZPos = 4,
};

// The SPLIT_THRESHOLD is the number of objects a node must contain before its
// children are generated. Increasing this number will decrease the number of
// octree nodes in the tree, which will use less memory but make culling less
// efficient.
template <class OBJ, class PREC>
inline DynamicOctree<OBJ, PREC>::DynamicOctree(const Point3<PREC>& cellCenterPos,
                                               const float         exclusionFactor):
        _children      (NULL),
        cellCenterPos  (cellCenterPos),
        exclusionFactor(exclusionFactor),
        _objects       (NULL)
{
}


template <class OBJ, class PREC>
inline DynamicOctree<OBJ, PREC>::~DynamicOctree()
{
    if (_children != NULL)
    {
        for (int i=0; i<8; ++i)
        {
            delete _children[i];
        }

        delete[] _children;
    }
    delete _objects;
}


template <class OBJ, class PREC>
inline void DynamicOctree<OBJ, PREC>::insertObject(const OBJ& obj, const PREC scale)
{
    // If the object can't be placed into this node's children, then put it here:
    if (limitingFactorPredicate(obj, exclusionFactor) || straddlingPredicate(cellCenterPos, obj, exclusionFactor) )
        add(obj);
    else
    {
        // If we haven't allocated child nodes yet, try to fit
        // the object in this node, even though it could be put
        // in a child. Only if there are more than SPLIT_THRESHOLD
        // objects in the node will we attempt to place the
        // object into a child node.  This is done in order
        // to avoid having the octree degenerate into one object
        // per node.
        if (_children == NULL)
        {
            // Make sure that there's enough room left in this node
            if (_objects != NULL && _objects->size() >= DynamicOctree<OBJ, PREC>::SPLIT_THRESHOLD)
                split(scale * 0.5f);
            add(obj);
        }
        else
            // We've already allocated child nodes; place the object
            // into the appropriate one.
            this->getChild(obj, cellCenterPos)->insertObject(obj, scale * (PREC) 0.5);
    }
}


template <class OBJ, class PREC>
inline void DynamicOctree<OBJ, PREC>::add(const OBJ& obj)
{
    if (_objects == NULL)
        _objects = new ObjectList;

    _objects->push_back(&obj);
}


template <class OBJ, class PREC>
inline void DynamicOctree<OBJ, PREC>::split(const PREC scale)
{
    _children = new DynamicOctree*[8];

    for (int i=0; i<8; ++i)
    {
        Point3<PREC> centerPos    = cellCenterPos;

        centerPos.x     += ((i & XPos) != 0) ? scale : -scale;
        centerPos.y     += ((i & YPos) != 0) ? scale : -scale;
        centerPos.z     += ((i & ZPos) != 0) ? scale : -scale;

        _children[i] = new DynamicOctree(centerPos,
                                         decayFunction(exclusionFactor));
    }
    sortIntoChildNodes();
}


// Sort this node's objects into objects that can remain here,
// and objects that should be placed into one of the eight
// child nodes.
template <class OBJ, class PREC>
inline void DynamicOctree<OBJ, PREC>::sortIntoChildNodes()
{
    unsigned int nKeptInParent = 0;

    for (unsigned int i=0; i<_objects->size(); ++i)
    {
        const OBJ& obj    = *(*_objects)[i];

        if (limitingFactorPredicate(obj, exclusionFactor) ||
            straddlingPredicate(cellCenterPos, obj, exclusionFactor) )
        {
            (*_objects)[nKeptInParent++] = (*_objects)[i];
        }
        else
            this->getChild(obj, cellCenterPos)->add(obj);
    }

    _objects->resize(nKeptInParent);
}


template <class OBJ, class PREC>
inline void DynamicOctree<OBJ, PREC>::rebuildAndSort(StaticOctree<OBJ, PREC>*& _staticNode, OBJ*& _sortedObjects)
{
    OBJ* _firstObject = _sortedObjects;

    if (_objects != NULL)
        for (typename ObjectList::const_iterator iter = _objects->begin(); iter != _objects->end(); ++iter)
        {
            *_sortedObjects++ = **iter;
        }

    unsigned int nObjects  = (unsigned int) (_sortedObjects - _firstObject);
    _staticNode            = new StaticOctree<OBJ, PREC>(cellCenterPos, exclusionFactor, _firstObject, nObjects);

    if (_children != NULL)
    {
        _staticNode->_children    = new StaticOctree<OBJ, PREC>*[8];

        for (int i=0; i<8; ++i)
            _children[i]->rebuildAndSort(_staticNode->_children[i], _sortedObjects);
    }
}


//MS VC++ wants this to be placed here:
template <class OBJ, class PREC>
const PREC StaticOctree<OBJ, PREC>::SQRT3 = (PREC) 1.732050807568877;


template <class OBJ, class PREC>
inline StaticOctree<OBJ, PREC>::StaticOctree(const Point3<PREC>& cellCenterPos,
                                             const float         exclusionFactor,
                                             OBJ*                _firstObject,
                                             unsigned int        nObjects):
    _children      (NULL),
    cellCenterPos  (cellCenterPos),
    exclusionFactor(exclusionFactor),
    _firstObject   (_firstObject),
    nObjects       (nObjects)
{
}


template <class OBJ, class PREC>
inline StaticOctree<OBJ, PREC>::~StaticOctree()
{
    if (_children != NULL)
    {
        for (int i=0; i<8; ++i)
            delete _children[i];

        delete[] _children;
    }
}


template <class OBJ, class PREC>
inline int StaticOctree<OBJ, PREC>::countChildren() const
{
    int count    = 0;

    for (int i=0; i<8; ++i)
        count    += _children != NULL ? 1 + _children[i]->countChildren() : 0;

    return count;
}


template <class OBJ, class PREC>
inline int StaticOctree<OBJ, PREC>::countObjects() const
{
    int count    = nObjects;

    if (_children != NULL)
        for (int i=0; i<8; ++i)
            count    += _children[i]->countObjects();

    return count;
}


template <class OBJ, class PREC>
void StaticOctree<OBJ, PREC>::computeStatistics(std::vector<OctreeLevelStatistics>& stats, unsigned int level)
{
    if (level >= stats.size())
    {
        while (level >= stats.size())
        {
            OctreeLevelStatistics levelStats;
            levelStats.nodeCount = 0;
            levelStats.objectCount = 0;
            levelStats.size = 0.0;
            stats.push_back(levelStats);
        }
    }

    stats[level].nodeCount++;
    stats[level].objectCount += nObjects;
    stats[level].size = 0.0;

    if (_children != NULL)
    {
        for (int i = 0; i < 8; i++)
            _children[i]->computeStatistics(stats, level + 1);
    }
}


#endif // _OCTREE_H_
