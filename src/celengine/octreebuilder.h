// octreebuilder.h
//
// Octree-based visibility determination for objects.
//
// Copyright (C) 2001-2024, Celestia Development Team
//
// Split from octree.h:
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
#include <vector>

#include <Eigen/Core>

#include "octree.h"

constexpr inline unsigned int OctreeXPos = 1;
constexpr inline unsigned int OctreeYPos = 2;
constexpr inline unsigned int OctreeZPos = 4;

// The DynamicOctree is built first by inserting objects from a database or
// catalog and is then 'compiled' into a StaticOctree. In the process of
// building the StaticOctree, the original object database is reorganized,
// with objects in the same octree node all placed adjacent to each other.
// This spatial sorting of the objects dramatically improves the performance
// of octree operations through much more coherent memory access.
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
    using ChildrenType = std::array<std::unique_ptr<DynamicOctree>, 8>;

    static const unsigned int SPLIT_THRESHOLD;

    static bool exceedsBrightnessThreshold(const OBJ&, float);
    static bool isStraddling(const PointType&, const OBJ&);
    static float applyDecay(float);

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
    getChild(obj, m_cellCenterPos)->insertObject(obj, scale * PREC(0.5));
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
    for (unsigned int i = 0U; i < 8U; ++i)
    {
        PointType centerPos = m_cellCenterPos
                            + PointType(((i & OctreeXPos) != 0U) ? scale : -scale,
                                        ((i & OctreeYPos) != 0U) ? scale : -scale,
                                        ((i & OctreeZPos) != 0U) ? scale : -scale);

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

        for (unsigned int i = 0U; i < 8U; ++i)
            (*staticNode->m_children)[i] = (*m_children)[i]->rebuildAndSort(sortedObjects);
    }

    return staticNode;
}
