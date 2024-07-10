// octree.h
//
// Octree-based visibility determination for objects.
//
// Copyright (C) 2001-2009, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <utility>

#include <Eigen/Core>
#include <Eigen/Geometry>

// The DynamicOctree and StaticOctree template arguments are:
// OBJ:  object hanging from the node,
// PREC: floating point precision of the culling operations at node level.
// The hierarchy of octree nodes is built using a single precision value (excludingFactor), which relates to an
// OBJ's limiting property defined by the octree particular specialization: ie. we use [absolute magnitude] for star octrees, etc.
// For details, see notes below.

template <class OBJ, class PREC>
class OctreeProcessor
{
public:
    virtual ~OctreeProcessor() = default;
    virtual void process(const OBJ& obj, PREC distance, float appMag) = 0;

protected:
    OctreeProcessor() = default;
};

template<class OBJ, class PREC>
class DynamicOctree;

template<class OBJ, class PREC>
class StaticOctree
{
public:
    using PointType = Eigen::Matrix<PREC, 3, 1>;
    using PlaneType = Eigen::Hyperplane<PREC, 3>;

    StaticOctree(const PointType& cellCenterPos,
                 const float      exclusionFactor,
                 OBJ*             _firstObject,
                 std::uint32_t    nObjects);

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
                               const PointType&            obsPosition,
                               const PlaneType*            frustumPlanes,
                               float                       limitingFactor,
                               PREC                        scale) const;

    void processCloseObjects(OctreeProcessor<OBJ, PREC>& processor,
                             const PointType&            obsPosition,
                             PREC                        boundingRadius,
                             PREC                        scale) const;

    std::uint32_t countChildren() const;
    std::uint32_t countObjects()  const;

private:
    using ChildrenType = std::array<std::unique_ptr<StaticOctree>, 8>;

    std::unique_ptr<ChildrenType> m_children;
    PointType m_cellCenterPos;
    float m_exclusionFactor;
    std::uint32_t m_nObjects;
    OBJ* m_firstObject;

    friend class DynamicOctree<OBJ, PREC>;
};

template<class OBJ, class PREC>
StaticOctree<OBJ, PREC>::StaticOctree(const PointType& cellCenterPos,
                                      float exclusionFactor,
                                      OBJ* firstObject,
                                      std::uint32_t nObjects):
    m_cellCenterPos(cellCenterPos),
    m_exclusionFactor(exclusionFactor),
    m_firstObject(firstObject),
    m_nObjects(nObjects)
{
}

template<class OBJ, class PREC>
std::uint32_t
StaticOctree<OBJ, PREC>::countChildren() const
{
    if (m_children == nullptr)
        return 0;

    std::uint32_t count    = 0;

    for (int i = 0; i < 8; ++i)
        count += UINT32_C(1) + (*m_children)[i]->countChildren();

    return count;
}

template<class OBJ, class PREC>
std::uint32_t
StaticOctree<OBJ, PREC>::countObjects() const
{
    std::uint32_t count = m_nObjects;

    if (m_children != nullptr)
        for (int i = 0; i < 8; ++i)
            count += (*m_children)[i]->countObjects();

    return count;
}

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

template<class OBJ, class PREC>
class DynamicOctree
{
public:
    using PointType = Eigen::Matrix<PREC, 3, 1>;

    DynamicOctree(const PointType& cellCenterPos,
                  float exclusionFactor);

    void insertObject(OBJ&, const PREC);
    std::unique_ptr<StaticOctree<OBJ, PREC>> rebuildAndSort(OBJ*&);

private:
    using ObjectList = std::vector<OBJ*>;

    static const unsigned int SPLIT_THRESHOLD;

    static bool exceedsBrightnessThreshold(const OBJ&, float);
    static bool isStraddling(const PointType&, const OBJ&);
    static float applyDecay(float);

private:
    using ChildrenType = std::array<std::unique_ptr<DynamicOctree>, 8>;

    void           add(OBJ&);
    void           split(const PREC);
    void           sortIntoChildNodes();
    DynamicOctree* getChild(const OBJ&, const PointType&) const;

    std::unique_ptr<ChildrenType> m_children;
    PointType m_cellCenterPos;
    float m_exclusionFactor;
    std::unique_ptr<ObjectList> m_objects;
};

// The SPLIT_THRESHOLD is the number of objects a node must contain before its
// children are generated. Increasing this number will decrease the number of
// octree nodes in the tree, which will use less memory but make culling less
// efficient.
template<class OBJ, class PREC>
DynamicOctree<OBJ, PREC>::DynamicOctree(const PointType& cellCenterPos,
                                        float exclusionFactor):
    m_cellCenterPos(cellCenterPos),
    m_exclusionFactor(exclusionFactor)
{
}

template<class OBJ, class PREC>
void
DynamicOctree<OBJ, PREC>::insertObject(OBJ& obj, const PREC scale)
{
    // If the object can't be placed into this node's children, then put it here:
    if (exceedsBrightnessThreshold(obj, m_exclusionFactor) || isStraddling(m_cellCenterPos, obj))
    {
        add(obj);
        return;
    }

    // If we haven't allocated child nodes yet, try to fit
    // the object in this node, even though it could be put
    // in a child. Only if there are more than SPLIT_THRESHOLD
    // objects in the node will we attempt to place the
    // object into a child node.  This is done in order
    // to avoid having the octree degenerate into one object
    // per node.
    if (m_children == nullptr)
    {
        if (m_objects == nullptr || m_objects->size() < SPLIT_THRESHOLD)
        {
            add(obj);
            return;
        }

        split(scale * 0.5f);
    }

    // We've already allocated child nodes; place the object
    // into the appropriate one.
    getChild(obj, m_cellCenterPos)->insertObject(obj, scale * (PREC) 0.5);
}

template<class OBJ, class PREC>
void
DynamicOctree<OBJ, PREC>::add(OBJ& obj)
{
    if (m_objects == nullptr)
        m_objects = std::make_unique<ObjectList>();

    m_objects->push_back(&obj);
}

template<class OBJ, class PREC>
void
DynamicOctree<OBJ, PREC>::split(const PREC scale)
{
    m_children = std::make_unique<ChildrenType>();
    for (int i = 0; i < 8; ++i)
    {
        PointType centerPos = m_cellCenterPos
                            + PointType(((i & XPos) != 0) ? scale : -scale,
                                        ((i & YPos) != 0) ? scale : -scale,
                                        ((i & ZPos) != 0) ? scale : -scale);

        (*m_children)[i] = std::make_unique<DynamicOctree>(centerPos, applyDecay(m_exclusionFactor));
    }

    sortIntoChildNodes();
}

// Sort this node's objects into objects that can remain here,
// and objects that should be placed into one of the eight
// child nodes.
template<class OBJ, class PREC>
void
DynamicOctree<OBJ, PREC>::sortIntoChildNodes()
{
    auto writeIt = m_objects->begin();
    auto endIt = m_objects->end();

    for (auto readIt = writeIt; readIt != endIt; ++readIt)
    {
        OBJ& obj = **readIt;
        if (exceedsBrightnessThreshold(obj, m_exclusionFactor) || isStraddling(m_cellCenterPos, obj))
        {
            *writeIt = *readIt;
            ++writeIt;
        }
        else
        {
            getChild(obj, m_cellCenterPos)->add(obj);
        }
    }

    m_objects->erase(writeIt, endIt);
}

template<class OBJ, class PREC>
std::unique_ptr<StaticOctree<OBJ, PREC>>
DynamicOctree<OBJ, PREC>::rebuildAndSort(OBJ*& sortedObjects)
{
    OBJ* firstObject = sortedObjects;

    if (m_objects != nullptr)
    {
        for (OBJ* obj : *m_objects)
        {
            *sortedObjects = std::move(*obj);
            ++sortedObjects;
        }
    }

    auto nObjects = static_cast<std::uint32_t>(sortedObjects - firstObject);
    auto staticNode = std::make_unique<StaticOctree<OBJ, PREC>>(m_cellCenterPos, m_exclusionFactor, firstObject, nObjects);

    if (m_children != nullptr)
    {
        staticNode->m_children = std::make_unique<typename StaticOctree<OBJ, PREC>::ChildrenType>();

        for (int i = 0; i < 8; ++i)
            (*staticNode->m_children)[i] = (*m_children)[i]->rebuildAndSort(sortedObjects);
    }

    return staticNode;
}
