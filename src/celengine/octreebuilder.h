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
#include "octree.h"

namespace celestia::engine
{

namespace detail
{

template<typename PREC>
struct OctreeBuilderNode
{
    using child_array = std::array<std::unique_ptr<OctreeBuilderNode>, 8>;
    using point_type = Eigen::Matrix<PREC, 3, 1>;

    explicit OctreeBuilderNode(const point_type&);
    ~OctreeBuilderNode();

    point_type center;
    std::vector<std::uint32_t> indices;
    std::unique_ptr<child_array> children;
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
reorderOctreeElements(std::vector<OBJ>& objects, std::vector<std::uint32_t>& indices)
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

    struct BuildVisitor;
    struct IndexMapVisitor;

    void addObject(std::uint32_t);
    bool checkStraddling(const point_type&, std::uint32_t);
    node_type* getChildNode(node_type*, std::uint32_t, std::uint32_t);
    void splitNode(node_type*, std::uint32_t);
    void buildIndexMap();

    template<typename VISITOR>
    void processNodes(VISITOR&) const;

    std::vector<object_type> m_objects;
    std::vector<std::uint32_t> m_indices;
    std::vector<float> m_thresholdMags;
    std::vector<precision_type> m_scales;
    std::unique_ptr<node_type> m_root;
    std::unique_ptr<std::vector<std::uint32_t>> m_excludedIndices;
    std::uint32_t m_nodeCount{ 1 };
};

template<typename TRAITS>
OctreeBuilder<TRAITS>::OctreeBuilder(std::vector<object_type>&& objects,
                                     precision_type rootSize,
                                     float rootMag) :
    m_objects(std::move(objects)),
    m_root(std::make_unique<node_type>(point_type::Zero()))
{
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
OctreeBuilder<TRAITS>::addObject(std::uint32_t idx)
{
    if ((TRAITS::getPosition(m_objects[idx]) - m_root->center).cwiseAbs().maxCoeff() > m_scales.front())
    {
        // If the object is outside the root note, put it in the list of excluded indices
        if (!m_excludedIndices)
            m_excludedIndices = std::make_unique<std::vector<std::uint32_t>>();
        m_excludedIndices->push_back(idx);
        return;
    }

    std::uint32_t depth = 0;
    node_type* node = m_root.get();
    for (;;)
    {
        if (depth >= m_thresholdMags.size())
            m_thresholdMags.push_back(TRAITS::decayMagnitude(m_thresholdMags.back()));

        float thresholdMag = m_thresholdMags[depth];
        if (TRAITS::getAbsMag(m_objects[idx]) <= thresholdMag || checkStraddling(node->center, idx))
        {
            node->indices.push_back(idx);
            return;
        }

        if (!node->children)
        {
            if (node->indices.size() < TRAITS::SplitThreshold)
            {
                node->indices.push_back(idx);
                return;
            }

            splitNode(node, depth);
        }

        ++depth;
        node = getChildNode(node, idx, depth);
    }
}

template<typename TRAITS>
bool
OctreeBuilder<TRAITS>::checkStraddling(const point_type& center, std::uint32_t idx)
{
    auto radius = TRAITS::getRadius(m_objects[idx]);
    return radius > 0 && (TRAITS::getPosition(m_objects[idx]) - center).cwiseAbs().minCoeff() < radius;
}

template<typename TRAITS>
typename OctreeBuilder<TRAITS>::node_type*
OctreeBuilder<TRAITS>::getChildNode(node_type* node,
                                    std::uint32_t idx,
                                    std::uint32_t depth)
{
    point_type position = TRAITS::getPosition(m_objects[idx]);
    int index = static_cast<int>(position.x() >= node->center.x()) |
                (static_cast<int>(position.y() >= node->center.y()) << 1) |
                (static_cast<int>(position.z() >= node->center.z()) << 2);
    auto& child = (*node->children)[index];
    if (!child)
    {
        if (depth >= m_scales.size())
            m_scales.push_back(m_scales.back() * precision_type(0.5));

        auto newSize = m_scales[depth];
        point_type newCenter = node->center
                             + newSize * point_type(((index & 1) << 1) - 1,
                                                    (index & 2) - 1,
                                                    ((index & 4) >> 1) - 1);
        child = std::make_unique<node_type>(newCenter);
        ++m_nodeCount;
    }

    return child.get();
}

template<typename TRAITS>
void
OctreeBuilder<TRAITS>::splitNode(node_type* node, std::uint32_t depth)
{
    const float thresholdMag = m_thresholdMags[depth];

    node->children = std::make_unique<typename node_type::child_array>();

    const auto end = node->indices.end();
    auto writeIt = node->indices.begin();
    for (auto readIt = node->indices.begin(); readIt != end; ++readIt)
    {
        if (TRAITS::getAbsMag(m_objects[*readIt]) <= thresholdMag || checkStraddling(node->center, *readIt))
        {
            *writeIt = *readIt;
            ++writeIt;
            continue;
        }

        getChildNode(node, *readIt, depth + 1)->indices.push_back(*readIt);
    }

    node->indices.erase(writeIt, end);
}

template<typename TRAITS>
template<typename VISITOR>
void
OctreeBuilder<TRAITS>::processNodes(VISITOR& visitor) const
{
    std::vector<std::pair<const node_type*, std::uint32_t>> nodeStack;
    nodeStack.emplace_back(m_root.get(), 0);

    while (!nodeStack.empty())
    {
        const node_type* node = nodeStack.back().first;
        std::uint32_t depth = nodeStack.back().second;
        nodeStack.pop_back();

        visitor.visit(node, depth);

        if (!node->children)
            continue;

        for (const auto& child : *node->children)
            nodeStack.emplace_back(child.get(), depth + 1);
    }
}

template<typename TRAITS>
struct OctreeBuilder<TRAITS>::IndexMapVisitor
{
    explicit IndexMapVisitor(OctreeBuilder*);

    void visit(const node_type*, std::uint32_t);

    OctreeBuilder* builder;
    std::uint32_t nextIdx{ 0 };
};

template<typename TRAITS>
OctreeBuilder<TRAITS>::IndexMapVisitor::IndexMapVisitor(OctreeBuilder* _builder) :
    builder(_builder)
{
    builder->m_indices.resize(builder->m_objects.size());
}

template<typename TRAITS>
void
OctreeBuilder<TRAITS>::IndexMapVisitor::visit(const node_type* node, std::uint32_t depth)
{
    for (std::uint32_t idx : node->indices)
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

    if (!m_excludedIndices)
        return;

    for (std::uint32_t idx : *m_excludedIndices)
    {
        m_indices[idx] = visitor.nextIdx;
        ++visitor.nextIdx;
    }
}

template<typename TRAITS>
struct OctreeBuilder<TRAITS>::BuildVisitor
{
    explicit BuildVisitor(OctreeBuilder*);

    void visit(const node_type*, std::uint32_t);

    OctreeBuilder* builder;
    std::vector<static_node_type> staticNodes;
    std::vector<std::uint32_t> prevByLevel;
    std::uint32_t topmostPopulated{ std::numeric_limits<std::uint32_t>::max() };
    std::uint32_t maxDepth{ 0 };
};

template<typename TRAITS>
OctreeBuilder<TRAITS>::BuildVisitor::BuildVisitor(OctreeBuilder* _builder) :
    builder(_builder)
{
    staticNodes.reserve(builder->m_nodeCount);
    builder->m_indices.clear();
    builder->m_indices.reserve(builder->m_objects.size());
}

template<typename TRAITS>
void
OctreeBuilder<TRAITS>::BuildVisitor::visit(const node_type* node, std::uint32_t depth)
{
    const auto idx = static_cast<std::uint32_t>(staticNodes.size());

    maxDepth = std::max(depth, maxDepth);

    while (prevByLevel.size() > depth)
    {
        staticNodes[prevByLevel.back()].afterSubtree = idx;
        prevByLevel.pop_back();
    }

    prevByLevel.push_back(idx);

    auto& staticNode = staticNodes.emplace_back(node->center, builder->m_scales[depth], depth);

    auto brightestIt = std::min_element(node->indices.begin(), node->indices.end(),
                                       [this](std::uint32_t a, std::uint32_t b)
                                       {
                                           return TRAITS::getAbsMag(builder->m_objects[a]) < TRAITS::getAbsMag(builder->m_objects[b]);
                                       });
    if (brightestIt == node->indices.end())
        return;

    if (depth < topmostPopulated)
        topmostPopulated = depth;

    staticNode.brightestMag = TRAITS::getAbsMag(builder->m_objects[*brightestIt]);
    staticNode.startOffset = static_cast<std::uint32_t>(builder->m_indices.size());
    staticNode.size = static_cast<std::uint32_t>(node->indices.size());

    std::copy(node->indices.begin(), node->indices.end(), std::back_inserter(builder->m_indices));

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

    if (m_excludedIndices)
        std::copy(m_excludedIndices->cbegin(), m_excludedIndices->cend(), std::back_inserter(m_indices));

    assert(m_indices.size() == m_objects.size());

    detail::reorderOctreeElements(m_objects, m_indices);
    return static_octree(std::move(visitor.staticNodes),
                         std::move(m_objects),
                         visitor.topmostPopulated,
                         visitor.maxDepth);
}

} // end namespace celestia::engine
