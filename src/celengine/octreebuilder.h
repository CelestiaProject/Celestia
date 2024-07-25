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

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

#include <Eigen/Core>

#include "octree.h"

namespace celestia::engine
{

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

    void insertObject(OBJ&, PREC);
    std::unique_ptr<StaticOctree<OBJ, PREC>> rebuildAndSort(PREC, OctreeObjectIndex) const;

private:
    using ObjectList = std::vector<OBJ*>;
    using ChildrenType = std::array<std::unique_ptr<DynamicOctree>, 8>;

    static const unsigned int SPLIT_THRESHOLD;

    static float        getMagnitude(const OBJ&);
    static bool         isStraddling(const PointType&, const OBJ&);
    static float        applyDecay(float);
    static unsigned int getChildIndex(const OBJ&, const PointType&);

    void           add(OBJ&);
    void           split(PREC);
    DynamicOctree* getChild(const OBJ&, PREC);

    OctreeNodeIndex countNodes() const;

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
DynamicOctree<OBJ, PREC>::insertObject(OBJ& obj, PREC scale)
{
    for (DynamicOctree* node = this;;)
    {
        if (getMagnitude(obj) <= node->m_exclusionFactor || isStraddling(node->m_cellCenterPos, obj))
        {
            node->add(obj);
            return;
        }

        // If we haven't allocated child nodes yet, try to fit
        // the object in this node, even though it could be put
        // in a child. Only if there are more than SPLIT_THRESHOLD
        // objects in the node will we attempt to place the
        // object into a child node.  This is done in order
        // to avoid having the octree degenerate into one object
        // per node.

        scale *= PREC(0.5);

        if (node->m_children == nullptr)
        {
            if (node->m_objects == nullptr || node->m_objects->size() < SPLIT_THRESHOLD)
            {
                node->add(obj);
                return;
            }

            node->split(scale);
        }

        node = node->getChild(obj, scale);
    }
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
DynamicOctree<OBJ, PREC>::split(PREC scale)
{
    assert(m_children == nullptr);
    m_children = std::make_unique<ChildrenType>();

    auto writeIt = m_objects->begin();
    auto endIt = m_objects->end();

    for (auto readIt = writeIt; readIt != endIt; ++readIt)
    {
        OBJ& obj = **readIt;
        if (getMagnitude(obj) <= m_exclusionFactor || isStraddling(m_cellCenterPos, obj))
        {
            *writeIt = *readIt;
            ++writeIt;
        }
        else
        {
            getChild(obj, scale)->add(obj);
        }
    }

    m_objects->erase(writeIt, endIt);
}

template<class OBJ, class PREC>
DynamicOctree<OBJ, PREC>*
DynamicOctree<OBJ, PREC>::getChild(const OBJ& obj, PREC scale)
{
    assert(m_children != nullptr);

    unsigned int childIndex = getChildIndex(obj, m_cellCenterPos);
    auto& result = (*m_children)[childIndex];
    if (result == nullptr)
    {
        PointType centerPos = m_cellCenterPos
                            + PointType(((childIndex & OctreeXPos) != 0U) ? scale : -scale,
                                        ((childIndex & OctreeYPos) != 0U) ? scale : -scale,
                                        ((childIndex & OctreeZPos) != 0U) ? scale : -scale);
        result = std::make_unique<DynamicOctree>(centerPos, applyDecay(m_exclusionFactor));
    }

    return result.get();
}

template<class OBJ, class PREC>
OctreeNodeIndex
DynamicOctree<OBJ, PREC>::countNodes() const
{
    OctreeNodeIndex result = 0;

    std::vector<const DynamicOctree*> nodeStack;
    nodeStack.push_back(this);
    while (!nodeStack.empty())
    {
        const DynamicOctree* node = nodeStack.back();
        nodeStack.pop_back();

        ++result;

        if (node->m_children == nullptr)
            continue;

        for (const auto& child : *node->m_children)
        {
            if (child != nullptr)
                nodeStack.push_back(child.get());
        }
    }

    return result;
}

template<class OBJ, class PREC>
std::unique_ptr<StaticOctree<OBJ, PREC>>
DynamicOctree<OBJ, PREC>::rebuildAndSort(PREC scale, OctreeObjectIndex objectCount) const
{
    auto staticOctree = std::make_unique<StaticOctree<OBJ, PREC>>();
    staticOctree->m_nodes.reserve(countNodes());
    staticOctree->m_objects.reserve(objectCount);

    std::vector<std::tuple<const DynamicOctree*, PREC, OctreeNodeIndex>> nodeStack;
    std::vector<OctreeNodeIndex> nodeIndexStack;
    nodeStack.emplace_back(this, scale, 0);

    while (!nodeStack.empty())
    {
        auto [node, nodeScale, depth] = nodeStack.back();
        nodeStack.pop_back();

        // any nodes in the node stack with a depth greater or equal to this one
        // will have this node as the node to jump to when skipping the subtree
        auto nodeIndex = static_cast<OctreeNodeIndex>(staticOctree->m_nodes.size());
        while (nodeIndexStack.size() > depth)
        {
            staticOctree->m_nodes[nodeIndexStack.back()].right = nodeIndex;
            nodeIndexStack.pop_back();
        }

        nodeIndexStack.push_back(nodeIndex);

        auto& staticNode = staticOctree->m_nodes.emplace_back(node->m_cellCenterPos, nodeScale);
        if (node->m_objects != nullptr && !node->m_objects->empty())
        {
            staticNode.first = static_cast<OctreeObjectIndex>(staticOctree->m_objects.size());
            staticNode.last = staticNode.first + static_cast<OctreeObjectIndex>(node->m_objects->size());

            for (OBJ* obj : *node->m_objects)
            {
                staticNode.brightFactor = std::min(staticNode.brightFactor, getMagnitude(*obj));
                staticOctree->m_objects.emplace_back(std::move(*obj));
            }

            // update parent node brightness factors, necessary in case they have no
            // stars
            for (OctreeNodeIndex parentDepth = 0; parentDepth < depth; ++parentDepth)
            {
                auto& parentNode = staticOctree->m_nodes[nodeIndexStack[parentDepth]];
                parentNode.brightFactor = std::min(parentNode.brightFactor, staticNode.brightFactor);
            }
        }

        if (node->m_children == nullptr)
            continue;

        for (const auto& child : *node->m_children)
        {
            if (child != nullptr)
                nodeStack.emplace_back(child.get(), nodeScale * PREC(0.5), depth + 1);
        }
    }

    return staticOctree;
}

} // end namespace celestia::engine
