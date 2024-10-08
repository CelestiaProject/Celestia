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
#include <type_traits>
#include <utility>
#include <vector>

#include <Eigen/Core>

#include <celutil/blockarray.h>
#include "octree.h"

namespace celestia::engine
{

namespace detail
{

template<class PREC>
struct DynamicOctreeNode
{
    using PointType = Eigen::Matrix<PREC, 3, 1>;
    using ChildrenType = std::array<OctreeNodeIndex, 8>;

    explicit DynamicOctreeNode(const PointType&);

    bool isStraddling(const PointType&, PREC);

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
DynamicOctreeNode<PREC>::isStraddling(const PointType& pos, PREC radius)
{
    return radius > PREC(0) && (pos - center).cwiseAbs().minCoeff() < radius;
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

    void insertObject(OctreeObjectIndex);
    void splitNode(NodeType&, OctreeDepthType);
    OctreeNodeIndex getChild(NodeType&, OctreeDepthType, const PointType&);
    void buildNode(StaticOctreeType&,
                   const NodeType&,
                   OctreeDepthType,
                   std::vector<OctreeNodeIndex>&);

    BlockArray<NodeType> m_nodes;
    STORAGE m_objects;
    std::vector<PrecisionType> m_sizes;
    std::vector<float> m_factors;
    std::vector<OctreeObjectIndex> m_excluded;
    OctreeObjectIndex m_splitThreshold;
};

template<class TRAITS, class STORAGE>
DynamicOctree<TRAITS, STORAGE>::DynamicOctree(STORAGE&& objects,
                                              const PointType& rootCenter,
                                              PrecisionType rootSize,
                                              float rootExclusionFactor,
                                              OctreeObjectIndex splitThreshold) :
    m_objects(std::move(objects)),
    m_splitThreshold(splitThreshold)
{
    m_nodes.emplace_back(rootCenter);
    m_sizes.emplace_back(rootSize);
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
    if ((pos - m_nodes[0].center).cwiseAbs().maxCoeff() > m_sizes[0])
    {
        m_excluded.push_back(idx);
        return;
    }

    OctreeNodeIndex nodeIdx = 0;
    OctreeDepthType depth = 0;
    for (;;)
    {
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
        if (node.children == nullptr)
        {
            if (node.objIndices.size() < m_splitThreshold)
            {
                node.objIndices.push_back(idx);
                return;
            }

            splitNode(node, depth);
        }

        ++depth;
        nodeIdx = getChild(node, depth, pos);
    }
}

template<class TRAITS, class STORAGE>
void
DynamicOctree<TRAITS, STORAGE>::splitNode(NodeType& node, OctreeDepthType depth)
{
    assert(node.children == nullptr);
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
        }
        else
        {
            NodeType& childNode = m_nodes[getChild(node, depth + 1, pos)];
            childNode.objIndices.push_back(*readIt);
        }
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
DynamicOctree<TRAITS, STORAGE>::getChild(NodeType& node, OctreeDepthType depth, const PointType& pos)
{
    assert(node.children != nullptr);

    auto childIndex = static_cast<unsigned int>(pos.x() >= node.center.x()) |
                      (static_cast<unsigned int>(pos.y() >= node.center.y()) << 1U) |
                      (static_cast<unsigned int>(pos.z() >= node.center.z()) << 2U);
    OctreeNodeIndex& result = (*node.children)[childIndex];
    if (result == InvalidOctreeNode)
    {
        result = static_cast<OctreeNodeIndex>(m_nodes.size());
        if (m_sizes.size() <= depth)
            m_sizes.push_back(m_sizes.back() * PrecisionType(0.5));
        PrecisionType scale = m_sizes[depth];
        PointType centerPos = node.center
                            + scale * PointType(static_cast<PrecisionType>(static_cast<int>((childIndex & 1U) << 1U) - 1),
                                                static_cast<PrecisionType>(static_cast<int>(childIndex & 2U) - 1),
                                                static_cast<PrecisionType>(static_cast<int>((childIndex & 4U) >> 1U) - 1));
        m_nodes.emplace_back(centerPos);
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

    std::vector<std::pair<OctreeNodeIndex, OctreeDepthType>> nodeStack;
    std::vector<OctreeNodeIndex> prevByDepth;
    nodeStack.emplace_back(0, 0);
    while (!nodeStack.empty())
    {
        auto [nodeIdx, depth] = nodeStack.back();
        nodeStack.pop_back();

        const NodeType& node = m_nodes[nodeIdx];
        buildNode(*staticOctree, node, depth, prevByDepth);
        if (depth < staticOctree->m_minPopulated && !node.objIndices.empty())
            staticOctree->m_minPopulated = depth;
        if (depth > staticOctree->m_maxDepth)
            staticOctree->m_maxDepth = depth;

        if (node.children == nullptr)
            continue;

        for (OctreeNodeIndex child : *node.children)
        {
            if (child != InvalidOctreeNode)
                nodeStack.emplace_back(child, depth + 1);
        }
    }

    for (OctreeObjectIndex objIndex : m_excluded)
        staticOctree->m_objects.emplace_back(std::move(m_objects[objIndex]));

    staticOctree->m_sizes = std::move(m_sizes);

    return staticOctree;
}

template<class TRAITS, class STORAGE>
void
DynamicOctree<TRAITS, STORAGE>::buildNode(StaticOctreeType& staticOctree,
                                          const NodeType& node,
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

    auto& staticNode = staticOctree.m_nodes.emplace_back(node.center, depth);

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
