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
#include <cassert>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <celutil/blockarray.h>
#include "octree.h"

namespace celestia::engine
{

namespace detail
{

constexpr inline unsigned int InvalidOctreeChildIndex = 8U;

template<class PREC>
struct DynamicOctreeNode
{
    using PointType = Eigen::Matrix<PREC, 3, 1>;
    using ChildrenType = std::array<OctreeNodeIndex, 8>;

    explicit DynamicOctreeNode(const PointType&);

    bool isStraddling(const PointType&, PREC) const;
    bool isSkippable(unsigned int& singleChild) const;

    PointType center;
    std::vector<OctreeObjectIndex> objIndices;
    std::unique_ptr<ChildrenType> children;
};

template<class PREC>
DynamicOctreeNode<PREC>::DynamicOctreeNode(const PointType& _center) :
    center(_center)
{
}

template<class PREC>
bool
DynamicOctreeNode<PREC>::isStraddling(const PointType& pos, PREC radius) const
{
    return radius > PREC(0) && (pos - center).cwiseAbs().minCoeff() < radius;
}

template<class PREC>
bool
DynamicOctreeNode<PREC>::isSkippable(unsigned int& singleChild) const
{
    singleChild = InvalidOctreeChildIndex;
    if (!objIndices.empty())
        return false;

    if (!children)
        return true;

    for (unsigned int childIndex = 0U; childIndex < 8U; ++childIndex)
    {
        if ((*children)[childIndex] == InvalidOctreeNode)
            continue;

        if (singleChild != InvalidOctreeChildIndex)
            return false;

        singleChild = childIndex;
    }

    return true;
}

template<class PREC>
Eigen::AlignedBox<PREC, 3>
subNodeBox(const Eigen::AlignedBox<PREC, 3>& nodeBox,
           const Eigen::Matrix<PREC, 3, 1>& center,
           unsigned int childIndex)
{
    Eigen::Matrix<PREC, 3, 1> minPos;
    Eigen::Matrix<PREC, 3, 1> maxPos;
    for (unsigned int axis = 0U; axis < 3U; ++axis)
    {
        if (childIndex & (1U << axis))
        {
            minPos[axis] = center[axis];
            maxPos[axis] = nodeBox.max()[axis];
        }
        else
        {
            minPos[axis] = nodeBox.min()[axis];
            maxPos[axis] = center[axis];
        }
    }

    return Eigen::AlignedBox<PREC, 3>(minPos, maxPos);
}

} // end namespace celestia::engine::detail

// The DynamicOctree is built first by inserting objects from a database or
// catalog and is then 'compiled' into a StaticOctree. In the process of
// building the StaticOctree, the original object database is reorganized,
// with objects in the same octree node all placed adjacent to each other.
// This spatial sorting of the objects dramatically improves the performance
// of octree operations through much more coherent memory access.

template<class TRAITS, class STORAGE>
class DynamicOctree
{
public:
    using StorageType = STORAGE;
    using ObjectType = typename TRAITS::ObjectType;
    using PrecisionType = typename TRAITS::PrecisionType;
    using PointType = Eigen::Matrix<PrecisionType, 3, 1>;
    using StaticOctreeType = StaticOctree<ObjectType, PrecisionType>;

    static_assert(std::is_same_v<decltype(std::declval<StorageType>()[std::declval<OctreeObjectIndex>()]), ObjectType&>, "Incorrect element type in STORAGE");
    static_assert(std::is_same_v<decltype(TRAITS::getPosition(std::declval<ObjectType>())), PointType>, "Incorrect return type for TRAITS::getPosition");
    static_assert(std::is_same_v<decltype(TRAITS::getRadius(std::declval<ObjectType>())), PrecisionType>, "Incorrect return type for TRAITS::getRadius");

    DynamicOctree(STORAGE&& objects,
                  const PointType& rootCenter,
                  PrecisionType rootSize,
                  float rootExclusionFactor,
                  OctreeObjectIndex splitThreshold);

    DynamicOctree(const DynamicOctree&) = delete;
    DynamicOctree& operator=(const DynamicOctree&) = delete;
    DynamicOctree(DynamicOctree&&) noexcept = default;
    DynamicOctree& operator=(DynamicOctree&&) noexcept = default;

    std::unique_ptr<StaticOctreeType> build();

private:
    using NodeType = detail::DynamicOctreeNode<PrecisionType>;
    using BoxType = Eigen::AlignedBox<PrecisionType, 3>;

    void insertObject(OctreeObjectIndex);
    void splitNode(NodeType&, const BoxType&, OctreeDepthType);
    OctreeNodeIndex getChild(NodeType&, BoxType&, const PointType&);
    void buildNode(StaticOctreeType&,
                   const NodeType&,
                   const BoxType&,
                   OctreeDepthType,
                   std::vector<OctreeNodeIndex>&);

    BlockArray<NodeType> m_nodes;
    STORAGE m_objects;
    std::vector<float> m_factors;
    std::vector<OctreeObjectIndex> m_excluded;
    BoxType m_rootBox;
    OctreeObjectIndex m_splitThreshold;
};

template<class TRAITS, class STORAGE>
DynamicOctree<TRAITS, STORAGE>::DynamicOctree(STORAGE&& objects,
                                              const PointType& rootCenter,
                                              PrecisionType rootSize,
                                              float rootExclusionFactor,
                                              OctreeObjectIndex splitThreshold) :
    m_objects(std::move(objects)),
    m_rootBox(rootCenter - PointType::Constant(rootSize), rootCenter + PointType::Constant(rootSize)),
    m_splitThreshold(splitThreshold)
{
    m_nodes.emplace_back(rootCenter);
    m_factors.emplace_back(rootExclusionFactor);

    for (OctreeObjectIndex i = 0, end = static_cast<OctreeObjectIndex>(m_objects.size()); i < end; ++i)
    {
        insertObject(i);
    }
}

template<class TRAITS, class STORAGE>
void
DynamicOctree<TRAITS, STORAGE>::insertObject(OctreeObjectIndex idx)
{
    const ObjectType& obj = m_objects[idx];
    PointType pos = TRAITS::getPosition(obj);
    if (!m_rootBox.contains(pos))
    {
        m_excluded.push_back(idx);
        return;
    }

    OctreeNodeIndex nodeIdx = 0;
    OctreeDepthType depth = 0;
    BoxType nodeBox = m_rootBox;
    for (;;)
    {
        assert(nodeBox.contains(pos));

        NodeType& node = m_nodes[nodeIdx];

        if (m_factors.size() <= depth)
            m_factors.push_back(TRAITS::applyDecay(m_factors.back()));

        // If the object can't be placed into this node's children, then put it here:
        if (TRAITS::getMagnitude(obj) <= m_factors[depth] || node.isStraddling(pos, TRAITS::getRadius(obj)))
        {
            node.objIndices.push_back(idx);
            return;
        }

        // If we haven't allocated child nodes yet, try to fit
        // the object in this node, even though it could be put
        // in a child. Only if there are more than SPLIT_THRESHOLD
        // objects in the node will we attempt to place the
        // object into a child node.  This is done in order
        // to avoid having the octree degenerate into one object
        // per node.
        if (!node.children)
        {
            if (node.objIndices.size() < m_splitThreshold)
            {
                node.objIndices.push_back(idx);
                return;
            }

            splitNode(node, nodeBox, depth);
        }

        ++depth;
        nodeIdx = getChild(node, nodeBox, pos);
    }
}

template<class TRAITS, class STORAGE>
void
DynamicOctree<TRAITS, STORAGE>::splitNode(NodeType& node, const BoxType& nodeBox, OctreeDepthType depth)
{
    assert(!node.children);
    node.children = std::make_unique<typename NodeType::ChildrenType>();
    node.children->fill(InvalidOctreeNode);

    auto writeIt = node.objIndices.begin();
    auto endIt = node.objIndices.end();

    for (auto readIt = writeIt; readIt != endIt; ++readIt)
    {
        const ObjectType& obj = m_objects[*readIt];
        PointType pos = TRAITS::getPosition(obj);
        if (TRAITS::getMagnitude(obj) <= m_factors[depth] || node.isStraddling(pos, TRAITS::getRadius(obj)))
        {
            *writeIt = *readIt;
            ++writeIt;
            continue;
        }

        auto childIndex =  static_cast<unsigned int>(pos.x() >= node.center.x())
                        | (static_cast<unsigned int>(pos.y() >= node.center.y()) << 1U)
                        | (static_cast<unsigned int>(pos.z() >= node.center.z()) << 2U);
        OctreeNodeIndex& childNodeIndex = (*node.children)[childIndex];
        if (childNodeIndex == InvalidOctreeNode)
        {
            childNodeIndex = static_cast<OctreeNodeIndex>(m_nodes.size());
            m_nodes.emplace_back(detail::subNodeBox(nodeBox, node.center, childIndex).center());
        }

        m_nodes[childNodeIndex].objIndices.push_back(*readIt);
    }

    if (writeIt == node.objIndices.begin())
    {
        // No objects retained: release heap allocation
        node.objIndices = std::vector<OctreeObjectIndex>();
    }
    else
    {
        node.objIndices.erase(writeIt, endIt);
    }
}

template<class TRAITS, class STORAGE>
OctreeNodeIndex
DynamicOctree<TRAITS, STORAGE>::getChild(NodeType& node, BoxType& nodeBox, const PointType& pos)
{
    assert(node.children);

    unsigned int childIndex = 0U;
    for (unsigned int axis = 0U; axis < 3U; ++axis)
    {
        if (pos[axis] >= node.center[axis])
        {
            childIndex |= 1U << axis;
            nodeBox.min()[axis] = node.center[axis];
        }
        else
        {
            nodeBox.max()[axis] = node.center[axis];
        }
    }

    OctreeNodeIndex& result = (*node.children)[childIndex];
    if (result == InvalidOctreeNode)
    {
        result = static_cast<OctreeNodeIndex>(m_nodes.size());
        m_nodes.emplace_back(nodeBox.center());
    }

    return result;
}

template<class TRAITS, class STORAGE>
std::unique_ptr<typename DynamicOctree<TRAITS, STORAGE>::StaticOctreeType>
DynamicOctree<TRAITS, STORAGE>::build()
{
    auto staticOctree = std::make_unique<StaticOctreeType>();
    staticOctree->m_nodes.reserve(m_nodes.size());
    staticOctree->m_objects.reserve(m_objects.size());

    std::vector<std::tuple<OctreeNodeIndex, OctreeDepthType, BoxType>> nodeStack;
    std::vector<OctreeNodeIndex> prevByDepth;
    nodeStack.emplace_back(0, 0, m_rootBox);
    while (!nodeStack.empty())
    {
        auto [nodeIdx, depth, nodeBox] = nodeStack.back();
        nodeStack.pop_back();

        const NodeType& node = m_nodes[nodeIdx];
        // Optimisation: any node which contains no objects and has a single
        // child node can be replaced with its child node.
        if (OctreeNodeIndex singleChild; node.isSkippable(singleChild))
        {
            if (singleChild != detail::InvalidOctreeChildIndex)
            {
                nodeStack.emplace_back((*node.children)[singleChild],
                                       depth,
                                       detail::subNodeBox(nodeBox, node.center, singleChild));
            }

            continue;
        }

        buildNode(*staticOctree, node, nodeBox, depth, prevByDepth);
        if (depth < staticOctree->m_minPopulated && !node.objIndices.empty())
            staticOctree->m_minPopulated = depth;
        if (depth > staticOctree->m_maxDepth)
            staticOctree->m_maxDepth = depth;

        if (!node.children)
            continue;

        for (unsigned int childIndex = 0U; childIndex < 8U; ++childIndex)
        {
            if (OctreeNodeIndex child = (*node.children)[childIndex]; child != InvalidOctreeNode)
            {
                nodeStack.emplace_back(child,
                                       depth + 1,
                                       detail::subNodeBox(nodeBox, node.center, childIndex));
            }
        }
    }

    for (OctreeObjectIndex objIndex : m_excluded)
        staticOctree->m_objects.emplace_back(std::move(m_objects[objIndex]));

    return staticOctree;
}

template<class TRAITS, class STORAGE>
void
DynamicOctree<TRAITS, STORAGE>::buildNode(StaticOctreeType& staticOctree,
                                          const NodeType& node,
                                          const BoxType& nodeBox,
                                          OctreeDepthType depth,
                                          std::vector<OctreeNodeIndex>& prevByDepth)
{
    // any nodes in the node stack with a depth greater or equal to this one
    // will have this node as the node to jump to when skipping the subtree
    auto staticNodeIdx = static_cast<OctreeNodeIndex>(staticOctree.m_nodes.size());
    while (prevByDepth.size() > depth)
    {
        staticOctree.m_nodes[prevByDepth.back()].right = staticNodeIdx;
        prevByDepth.pop_back();
    }

    prevByDepth.push_back(staticNodeIdx);

    PrecisionType nodeSize = nodeBox.sizes().maxCoeff() / PrecisionType{ 2 };
    auto& staticNode = staticOctree.m_nodes.emplace_back(node.center, nodeSize, depth);

    if (node.objIndices.empty())
        return;

    staticNode.first = static_cast<OctreeObjectIndex>(staticOctree.m_objects.size());
    staticNode.last = staticNode.first + static_cast<OctreeObjectIndex>(node.objIndices.size());

    // move the objects into the sorted octree, keep track of brightest
    for (OctreeObjectIndex objIdx : node.objIndices)
    {
        ObjectType& obj = m_objects[objIdx];
        staticNode.brightFactor = std::min(staticNode.brightFactor, TRAITS::getMagnitude(obj));
        staticOctree.m_objects.emplace_back(std::move(obj));
    }

    // update parent node brightness factors, in case node was empty or
    // (less likely) filled with objects straddling the center
    for (OctreeNodeIndex parentDepth = depth; parentDepth-- > 0;)
    {
        auto& parentNode = staticOctree.m_nodes[prevByDepth[parentDepth]];
        if (parentNode.brightFactor <= staticNode.brightFactor)
            break;

        parentNode.brightFactor = staticNode.brightFactor;
    }
}

template<typename TRAITS, typename STORAGE>
inline std::unique_ptr<DynamicOctree<TRAITS, STORAGE>>
makeDynamicOctree(STORAGE&& objects,
                  const Eigen::Matrix<typename TRAITS::PrecisionType, 3, 1>& rootCenter,
                  typename TRAITS::PrecisionType rootSize,
                  float rootExclusionFactor,
                  OctreeObjectIndex splitThreshold)
{
    static_assert(!std::is_reference_v<STORAGE>, "makeDynamicOctree must be called with rvalue for objects parameter");
    return std::make_unique<DynamicOctree<TRAITS, STORAGE>>(std::forward<STORAGE>(objects),
                                                            rootCenter,
                                                            rootSize,
                                                            rootExclusionFactor,
                                                            splitThreshold);
}

} // end namespace celestia::engine
