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

#include <cstdint>
#include <limits>
#include <vector>

#include <Eigen/Core>

#include <celutil/array_view.h>

namespace celestia::engine
{

namespace detail
{

constexpr inline std::uint32_t OctreeInvalidIndex = std::numeric_limits<std::uint32_t>::max();

template<typename PREC>
struct OctreeNode
{
    using point_type = Eigen::Matrix<PREC, 3, 1>;

    OctreeNode(const point_type&, PREC, std::uint32_t);

    point_type center;
    PREC scale;
    float brightestMag{ std::numeric_limits<float>::max() };
    std::uint32_t depth;
    std::uint32_t afterSubtree{ OctreeInvalidIndex };
    std::uint32_t startOffset{ 0 };
    std::uint32_t size{ 0 };
};

template<typename PREC>
OctreeNode<PREC>::OctreeNode(const point_type& _center, PREC _scale, std::uint32_t _depth) :
    center(_center),
    scale(_scale),
    depth(_depth)
{
}

} // end namespace celestia::engine::detail

template<typename TRAITS>
class OctreeBuilder;

template<typename OBJ, typename PREC>
class Octree
{
public:
    ~Octree() = default;

    Octree(const Octree&) = delete;
    Octree& operator=(const Octree&) = delete;
    Octree(Octree&&) noexcept = default;
    Octree& operator=(Octree&&) noexcept = default;

    celestia::util::array_view<OBJ> objects() const;
    std::uint32_t size() const;

    const OBJ& operator[](std::uint32_t) const;

    template<typename VISITOR>
    void processDepthFirst(VISITOR&) const;

    template<typename VISITOR>
    void processBreadthFirst(VISITOR&) const;

private:
    using node_type = detail::OctreeNode<PREC>;

    Octree(std::vector<node_type>&&, std::vector<OBJ>&&, std::uint32_t, std::uint32_t);

    template<typename VISITOR>
    bool processDepth(VISITOR&, std::uint32_t) const;

    std::vector<node_type> m_nodes;
    std::vector<OBJ> m_objects;
    std::uint32_t m_topmostPopulated;
    std::uint32_t m_maxDepth;

    template<typename>
    friend class OctreeBuilder;
};

template<typename OBJ, typename PREC>
Octree<OBJ, PREC>::Octree(std::vector<node_type>&& nodes,
                          std::vector<OBJ>&& objects,
                          std::uint32_t topmostPopulated,
                          std::uint32_t maxDepth) :
    m_nodes(std::move(nodes)),
    m_objects(std::move(objects)),
    m_topmostPopulated(topmostPopulated),
    m_maxDepth(maxDepth)
{
}

template<typename OBJ, typename PREC>
celestia::util::array_view<OBJ>
Octree<OBJ, PREC>::objects() const
{
    return m_objects;
}

template<typename OBJ, typename PREC>
std::uint32_t
Octree<OBJ, PREC>::size() const
{
    return static_cast<std::uint32_t>(m_objects.size());
}

template<typename OBJ, typename PREC>
const OBJ&
Octree<OBJ, PREC>::operator[](std::uint32_t n) const
{
    return m_objects[n];
}

template<typename OBJ, typename PREC>
template<typename VISITOR>
void
Octree<OBJ, PREC>::processDepthFirst(VISITOR& visitor) const
{
    std::uint32_t idx = 0;
    while (idx < m_nodes.size())
    {
        const auto& node = m_nodes[idx];
        if (!visitor.checkNode(node.center, node.size, node.brightestMag))
        {
            idx = node.afterSubtree;
            continue;
        }

        if (node.size > 0)
        {
            visitor.process(celestia::util::array_view<OBJ>(m_objects.data() + node.startOffset,
                                                            node.size));
        }

        ++idx;
    }
}

template<typename OBJ, typename PREC>
template<typename VISITOR>
void
Octree<OBJ, PREC>::processBreadthFirst(VISITOR& visitor) const
{
    // Use iterative depth-first search to simulate breadth-first search.
    // We start at the depth of the topmost populated node.
    std::uint32_t depth = m_topmostPopulated;
    while (depth <= m_maxDepth && processDepth(visitor, depth))
        ++depth;
}

template<typename OBJ, typename PREC>
template<typename VISITOR>
bool
Octree<OBJ, PREC>::processDepth(VISITOR& visitor, std::uint32_t depth) const
{
    std::uint32_t idx = 0;
    bool result = false;
    while (idx < m_nodes.size())
    {
        const auto& node = m_nodes[idx];
        if (!visitor.checkNode(node.center, node.size, node.brightestMag))
        {
            idx = node.afterSubtree;
        }
        else if (node.depth < depth)
        {
            // Do not process objects handled in previous iterations, just
            // navigate to the node's children.
            idx += 1;
        }
        else
        {
            // We have reached the requested depth, so report completion
            result = true;
            if (node.size > 0)
            {
                visitor.process(celestia::util::array_view<OBJ>(m_objects.data() + node.startOffset,
                                                                node.size));
            }

            // We've reached the maximum processing depth, so skip any
            // children of this node.
            idx = node.afterSubtree;
        }
    }

    return result;
}

} // end namespace celestia::engine
