// octree.h
//
// Octree-based visibility determination for objects.
//
// Copyright (C) 2001-2024, Celestia Development Team
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

namespace celestia::engine
{

using OctreeNodeIndex = std::uint32_t;
using OctreeObjectIndex = std::uint32_t;
using OctreeDepthType = std::uint32_t;

constexpr inline OctreeNodeIndex InvalidOctreeNode = UINT32_MAX;

namespace detail
{

template<class PREC>
struct StaticOctreeNode
{
    using PointType = Eigen::Matrix<PREC, 3, 1>;

    StaticOctreeNode(const PointType&, OctreeDepthType);

    PointType center;
    OctreeDepthType depth;
    OctreeNodeIndex right{ InvalidOctreeNode };
    OctreeObjectIndex first{ 0 };
    OctreeObjectIndex last{ 0 };
    float brightFactor{ 1000.0 };
};

template<class PREC>
StaticOctreeNode<PREC>::StaticOctreeNode(const PointType& _center,
                                         OctreeDepthType _depth) :
    center(_center),
    depth(_depth)
{
}

} // end namespace celestia::engine::detail

template <class OBJ, class PREC>
class OctreeProcessor
{
public:
    virtual ~OctreeProcessor() = default;
    virtual void process(const OBJ& obj, PREC distance, float appMag) = 0;

protected:
    OctreeProcessor() = default;
};

template<class TRAITS, class STORAGE>
class DynamicOctree;

// The StaticOctree template arguments are:
// OBJ:  object hanging from the node,
// PREC: floating point precision of the culling operations at node level.
// The hierarchy of octree nodes is built using a single precision value (excludingFactor), which relates to an
// OBJ's limiting property defined by the octree particular specialization: ie. we use [absolute magnitude] for star octrees, etc.
// For details, see notes below.

template<class OBJ, class PREC>
class StaticOctree
{
public:
    using PointType = Eigen::Matrix<PREC, 3, 1>;

    StaticOctree() = default;
    ~StaticOctree() = default;

    StaticOctree(const StaticOctree&) = delete;
    StaticOctree& operator=(const StaticOctree&) = delete;
    StaticOctree(StaticOctree&&) noexcept = default;
    StaticOctree& operator=(StaticOctree&&) noexcept = default;

    template<typename PROCESSOR>
    void processDepthFirst(PROCESSOR&) const;

    template<typename PROCESSOR>
    void processBreadthFirst(PROCESSOR&) const;

    OctreeObjectIndex size() const;
    OctreeNodeIndex nodeCount() const;

    OBJ& operator[](OctreeObjectIndex);
    const OBJ& operator[](OctreeObjectIndex) const;

private:
    using NodeType = detail::StaticOctreeNode<PREC>;

    template<typename PROCESSOR>
    bool processToDepth(PROCESSOR&, OctreeDepthType) const;

    std::vector<NodeType> m_nodes;
    std::vector<OBJ> m_objects;
    std::vector<PREC> m_sizes;
    OctreeDepthType m_minPopulated{ UINT32_MAX };
    OctreeDepthType m_maxDepth{ 0 };

    template<class TRAITS, class STORAGE>
    friend class DynamicOctree;
};

template<class OBJ, class PREC>
template<typename PROCESSOR>
void
StaticOctree<OBJ, PREC>::processDepthFirst(PROCESSOR& processor) const
{
    OctreeNodeIndex nodeIdx = 0;
    const OctreeNodeIndex endIdx = nodeCount();
    while (nodeIdx < endIdx)
    {
        const NodeType& node = m_nodes[nodeIdx];
        if (!processor.checkNode(node.center, m_sizes[node.depth], node.brightFactor))
        {
            nodeIdx = node.right;
            continue;
        }

        for (OctreeObjectIndex idx = node.first; idx < node.last; ++idx)
            processor.process(m_objects[idx]);

        ++nodeIdx;
    }
}

// Simulate a breadth-first search by doing an interative depth-first search,
// starting at the minimum populated depth. The search terminates when no nodes
// at the requested depth are reached.
template<class OBJ, class PREC>
template<typename PROCESSOR>
void
StaticOctree<OBJ, PREC>::processBreadthFirst(PROCESSOR& processor) const
{
    for (OctreeDepthType depth = m_minPopulated; depth <= m_maxDepth; ++depth)
    {
        if (!processToDepth(processor, depth))
            break;
    }
}

template<class OBJ, class PREC>
template<typename PROCESSOR>
bool
StaticOctree<OBJ, PREC>::processToDepth(PROCESSOR& processor, OctreeDepthType depth) const
{
    OctreeNodeIndex nodeIdx = 0;
    const OctreeNodeIndex endIdx = nodeCount();
    bool result = false;
    while (nodeIdx < endIdx)
    {
        const NodeType& node = m_nodes[nodeIdx];
        if (!processor.checkNode(node.center, m_sizes[node.depth], node.brightFactor))
        {
            nodeIdx = node.right;
        }
        else if (node.depth == depth)
        {
            result = true;
            for (OctreeObjectIndex idx = node.first; idx < node.last; ++idx)
                processor.process(m_objects[idx]);
            nodeIdx = node.right;
        }
        else
        {
            ++nodeIdx;
        }
    }

    return result;
}

template<class OBJ, class PREC>
OctreeObjectIndex
StaticOctree<OBJ, PREC>::size() const
{
    return static_cast<OctreeObjectIndex>(m_objects.size());
}

template<class OBJ, class PREC>
OctreeNodeIndex
StaticOctree<OBJ, PREC>::nodeCount() const
{
    return static_cast<OctreeNodeIndex>(m_nodes.size());
}

template<class OBJ, class PREC>
OBJ&
StaticOctree<OBJ, PREC>::operator[](OctreeObjectIndex idx)
{
    return m_objects[idx];
}

template<class OBJ, class PREC>
const OBJ&
StaticOctree<OBJ, PREC>::operator[](OctreeObjectIndex idx) const
{
    return m_objects[idx];
}

} // end namespace celestia::engine
