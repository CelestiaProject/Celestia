#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include <Eigen/Core>

#include <celutil/array_view.h>
#include <celutil/blockarray.h>
#include "octree.h"

namespace celestia::engine
{

namespace detail
{

template<typename PREC>
struct OctreeBuilderNode
{
    using point_type = Eigen::Matrix<PREC, 3, 1>;

    explicit OctreeBuilderNode(const point_type&);
    ~OctreeBuilderNode();

    OctreeBuilderNode(const OctreeBuilderNode&) = delete;
    OctreeBuilderNode& operator=(const OctreeBuilderNode&) = delete;
    OctreeBuilderNode(OctreeBuilderNode&&) noexcept = default;
    OctreeBuilderNode& operator=(OctreeBuilderNode&&) noexcept = default;

    point_type center;
    OctreeNodeIndexType childrenIndex{ OctreeNodeInvalidIndex };
    std::vector<OctreeIndexType> indices;
};

template<typename PREC>
OctreeBuilderNode<PREC>::OctreeBuilderNode(const point_type& _center) :
    center(_center)
{
}

template<typename PREC>
OctreeBuilderNode<PREC>::~OctreeBuilderNode() = default;

template<typename OBJ>
void
reorderOctreeElements(std::vector<OBJ>& objects, std::vector<OctreeIndexType>& indices)
{
    // In-place reordering

}

} // end namespace celestia::engine::detail

template<typename TRAITS>
class OctreeBuilder
{
public:
    using object_type = typename TRAITS::object_type;
    using precision_type = typename decltype(TRAITS::getPosition(std::declval<const object_type&>()))::Scalar;
    using point_type = Eigen::Matrix<precision_type, 3, 1>;
    using static_octree = Octree<object_type, precision_type>;

    OctreeBuilder(std::vector<object_type>&& objects, precision_type rootSize, float rootMag);

    std::vector<object_type>& objects();

    celestia::util::array_view<std::uint32_t> indices() const;
    static_octree build();

private:
    using node_type = detail::OctreeBuilderNode<precision_type>;
    using static_node_type = detail::OctreeNode<precision_type>;
    using children_type = std::array<detail::OctreeNodeIndexType, 8>;

    struct BuildVisitor;
    struct IndexMapVisitor;

    void addObject(OctreeIndexType);
    bool checkStraddling(const point_type&, OctreeIndexType);
    detail::OctreeNodeIndexType getChildIndex(node_type&, OctreeIndexType, std::uint32_t);
    void splitNode(node_type&, std::uint32_t);
    void buildIndexMap();

    template<typename VISITOR>
    void processNodes(VISITOR&) const;

    std::vector<object_type> m_objects;
    std::vector<std::uint32_t> m_indices;
    std::vector<float> m_thresholdMags;
    std::vector<precision_type> m_scales;

    std::vector<std::uint32_t> m_excludedIndices;

    // We store the nodes in a BlockArray rather than holding them in smart
    // pointers to avoid recursive destructor calls.
    BlockArray<node_type, 256> m_nodes;
    BlockArray<children_type, 256> m_children;
};

template<typename TRAITS>
OctreeBuilder<TRAITS>::OctreeBuilder(std::vector<object_type>&& objects,
                                     precision_type rootSize,
                                     float rootMag) :
    m_objects(std::move(objects))
{
    m_nodes.emplace_back(point_type::Zero());
    m_objects.shrink_to_fit();
    m_thresholdMags.push_back(rootMag);
    m_scales.push_back(rootSize);

    for (std::uint32_t i = 0; i < objects.size(); ++i)
        addObject(i);

    buildIndexMap();
}

template<typename TRAITS>
std::vector<typename OctreeBuilder<TRAITS>::object_type>&
OctreeBuilder<TRAITS>::objects()
{
    return m_objects;
}

template<typename TRAITS>
celestia::util::array_view<std::uint32_t>
OctreeBuilder<TRAITS>::indices() const
{
    return m_indices;
}

template<typename TRAITS>
void
OctreeBuilder<TRAITS>::addObject(OctreeIndexType idx)
{
    if ((TRAITS::getPosition(m_objects[idx]) - m_nodes.front().center).cwiseAbs().maxCoeff() > m_scales.front())
    {
        // If the object is outside the root note, put it in the list of excluded indices
        m_excludedIndices.push_back(idx);
        return;
    }

    std::uint32_t depth = 0;
    detail::OctreeNodeIndexType nodeIndex = 0;
    for (;;)
    {
        node_type& node = m_nodes[nodeIndex];

        if (depth >= m_thresholdMags.size())
            m_thresholdMags.push_back(TRAITS::decayMagnitude(m_thresholdMags.back()));

        float thresholdMag = m_thresholdMags[depth];
        if (TRAITS::getAbsMag(m_objects[idx]) <= thresholdMag || checkStraddling(node.center, idx))
        {
            node.indices.push_back(idx);
            return;
        }

        if (node.childrenIndex == detail::OctreeNodeInvalidIndex)
        {
            if (node.indices.size() < TRAITS::SplitThreshold)
            {
                node.indices.push_back(idx);
                return;
            }

            splitNode(node, depth);
        }

        ++depth;
        nodeIndex = getChildIndex(node, idx, depth);
    }
}

template<typename TRAITS>
bool
OctreeBuilder<TRAITS>::checkStraddling(const point_type& center, OctreeIndexType idx)
{
    auto radius = TRAITS::getRadius(m_objects[idx]);
    return radius > 0 && (TRAITS::getPosition(m_objects[idx]) - center).cwiseAbs().minCoeff() < radius;
}

template<typename TRAITS>
detail::OctreeNodeIndexType
OctreeBuilder<TRAITS>::getChildIndex(node_type& node,
                                     OctreeIndexType idx,
                                     std::uint32_t depth)
{
    assert(node.childrenIndex != detail::OctreeNodeInvalidIndex);

    point_type position = TRAITS::getPosition(m_objects[idx]);
    int index = static_cast<int>(position.x() >= node.center.x()) |
                (static_cast<int>(position.y() >= node.center.y()) << 1) |
                (static_cast<int>(position.z() >= node.center.z()) << 2);

    detail::OctreeNodeIndexType& childIndex = m_children[node.childrenIndex][index];
    if (childIndex == detail::OctreeNodeInvalidIndex)
    {
        if (depth >= m_scales.size())
            m_scales.push_back(m_scales.back() * precision_type(0.5));

        auto newSize = m_scales[depth];
        point_type newCenter = node.center
                             + newSize * point_type(((index & 1) << 1) - 1,
                                                    (index & 2) - 1,
                                                    ((index & 4) >> 1) - 1);

        childIndex = static_cast<detail::OctreeNodeIndexType>(m_nodes.size());
        m_nodes.emplace_back(newCenter);
    }

    return childIndex;
}

template<typename TRAITS>
void
OctreeBuilder<TRAITS>::splitNode(node_type& node, std::uint32_t depth)
{
    assert(node.childrenIndex == detail::OctreeNodeInvalidIndex);

    const float thresholdMag = m_thresholdMags[depth];

    node.childrenIndex = static_cast<detail::OctreeNodeIndexType>(m_children.size());
    m_children.emplace_back().fill(detail::OctreeNodeInvalidIndex);

    const auto end = node.indices.end();
    auto writeIt = node.indices.begin();
    for (auto readIt = node.indices.begin(); readIt != end; ++readIt)
    {
        if (TRAITS::getAbsMag(m_objects[*readIt]) <= thresholdMag || checkStraddling(node.center, *readIt))
        {
            *writeIt = *readIt;
            ++writeIt;
            continue;
        }

        detail::OctreeNodeIndexType childIndex = getChildIndex(node, *readIt, depth + 1);
        m_nodes[childIndex].indices.push_back(*readIt);
    }

    node.indices.erase(writeIt, end);
}

template<typename TRAITS>
template<typename VISITOR>
void
OctreeBuilder<TRAITS>::processNodes(VISITOR& visitor) const
{
    // Node stack: (nodeIndex, depth)
    std::vector<std::pair<detail::OctreeNodeIndexType, std::uint32_t>> nodeStack;
    nodeStack.emplace_back(0, 0);

    while (!nodeStack.empty())
    {
        detail::OctreeNodeIndexType nodeIndex = nodeStack.back().first;
        std::uint32_t depth = nodeStack.back().second;
        nodeStack.pop_back();

        const node_type& node = m_nodes[nodeIndex];
        visitor.visit(node, depth);

        if (node.childrenIndex == detail::OctreeNodeInvalidIndex)
            continue;

        for (detail::OctreeNodeIndexType childIndex : m_children[node.childrenIndex])
            nodeStack.emplace_back(childIndex, depth + 1);
    }
}

template<typename TRAITS>
struct OctreeBuilder<TRAITS>::IndexMapVisitor
{
    explicit IndexMapVisitor(OctreeBuilder*);

    void visit(const node_type&, std::uint32_t);

    OctreeBuilder* builder;
    OctreeIndexType nextIdx{ 0 };
};

template<typename TRAITS>
OctreeBuilder<TRAITS>::IndexMapVisitor::IndexMapVisitor(OctreeBuilder* _builder) :
    builder(_builder)
{
    builder->m_indices.resize(builder->m_objects.size());
}

template<typename TRAITS>
void
OctreeBuilder<TRAITS>::IndexMapVisitor::visit(const node_type& node, std::uint32_t /* depth */)
{
    for (OctreeIndexType idx : node.indices)
    {
        builder->m_indices[idx] = nextIdx;
        ++nextIdx;
    }
}

template<typename TRAITS>
void
OctreeBuilder<TRAITS>::buildIndexMap()
{
    IndexMapVisitor visitor(this);
    processNodes(visitor);

    for (OctreeIndexType idx : m_excludedIndices)
    {
        m_indices[idx] = visitor.nextIdx;
        ++visitor.nextIdx;
    }
}

template<typename TRAITS>
struct OctreeBuilder<TRAITS>::BuildVisitor
{
    explicit BuildVisitor(OctreeBuilder*);

    void visit(const node_type&, std::uint32_t);

    OctreeBuilder* builder;
    std::vector<static_node_type> staticNodes;
    std::vector<detail::OctreeNodeIndexType> prevByLevel;
    std::uint32_t topmostPopulated{ std::numeric_limits<std::uint32_t>::max() };
    std::uint32_t maxDepth{ 0 };
};

template<typename TRAITS>
OctreeBuilder<TRAITS>::BuildVisitor::BuildVisitor(OctreeBuilder* _builder) :
    builder(_builder)
{
    staticNodes.reserve(builder->m_nodes.size());
    builder->m_indices.clear();
    builder->m_indices.reserve(builder->m_objects.size());
}

template<typename TRAITS>
void
OctreeBuilder<TRAITS>::BuildVisitor::visit(const node_type& node, std::uint32_t depth)
{
    const auto idx = static_cast<detail::OctreeNodeIndexType>(staticNodes.size());

    maxDepth = std::max(depth, maxDepth);

    // When skipping the subtree of the last-visited node at a depth greater
    // than or equal to the depth of the current node, the visitor should skip
    // ahead to the current node.
    while (prevByLevel.size() > depth)
    {
        staticNodes[prevByLevel.back()].afterSubtree = idx;
        prevByLevel.pop_back();
    }

    prevByLevel.push_back(idx);

    auto& staticNode = staticNodes.emplace_back(node.center, builder->m_scales[depth], depth);

    // Find the brightest object contained in the current node.
    auto brightestIt = std::min_element(node.indices.begin(), node.indices.end(),
                                       [this](OctreeIndexType a, OctreeIndexType b)
                                       {
                                           return TRAITS::getAbsMag(builder->m_objects[a]) < TRAITS::getAbsMag(builder->m_objects[b]);
                                       });
    if (brightestIt == node.indices.end())
        return;

    if (depth < topmostPopulated)
        topmostPopulated = depth;

    staticNode.brightestMag = TRAITS::getAbsMag(builder->m_objects[*brightestIt]);

    staticNode.startOffset = static_cast<OctreeIndexType>(builder->m_indices.size());
    staticNode.size = static_cast<OctreeIndexType>(node.indices.size());

    std::copy(node.indices.begin(), node.indices.end(), std::back_inserter(builder->m_indices));

    // If there are nodes which contain only objects straddling the split
    // boundary, a node may contain brighter objects than any of its
    // ancestors. To account for this, update the brightest magnitude stored
    // in this node's ancestors.
    for (std::uint32_t i = depth; i-- > 0;)
    {
        auto& parentNode = staticNodes[prevByLevel[i]];
        if (parentNode.brightestMag <= staticNode.brightestMag)
            break;

        parentNode.brightestMag = staticNode.brightestMag;
    }
}

template<typename TRAITS>
typename OctreeBuilder<TRAITS>::static_octree
OctreeBuilder<TRAITS>::build()
{
    BuildVisitor visitor(this);
    processNodes(visitor);

    std::copy(m_excludedIndices.cbegin(), m_excludedIndices.cend(), std::back_inserter(m_indices));

    assert(m_indices.size() == m_objects.size());

    detail::reorderOctreeElements(m_objects, m_indices);
    return static_octree(std::move(visitor.staticNodes),
                         std::move(m_objects),
                         visitor.topmostPopulated,
                         visitor.maxDepth);
}

} // end namespace celestia::engine
