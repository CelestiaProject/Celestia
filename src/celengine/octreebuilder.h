#pragma once

#include <algorithm>
#include <array>
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

}

} // end namespace celestia::engine::detail

template<typename OBJ, typename TRAITS>
class OctreeBuilder
{
public:
    using precision_type = typename decltype(TRAITS::getPosition(std::declval<const OBJ&>()))::Scalar;
    using point_type = Eigen::Matrix<precision_type, 3, 1>;
    using static_octree = Octree<OBJ, precision_type>;

    OctreeBuilder(std::vector<OBJ>&& objects, precision_type rootSize, float rootMag);

    std::vector<OBJ>& objects();

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
    void processNodes(VISITOR&);

    std::vector<OBJ> m_objects;
    std::vector<std::uint32_t> m_indices;
    std::vector<float> m_thresholdMags;
    std::vector<precision_type> m_scales;
    std::unique_ptr<node_type> m_root;
    std::uint32_t m_nodeCount{ 1 };
};

template<typename OBJ, typename TRAITS>
OctreeBuilder<OBJ, TRAITS>::OctreeBuilder(std::vector<OBJ>&& objects,
                                          precision_type rootSize,
                                          float rootMag) :
    m_objects(std::move(objects)),
    m_root(std::make_unique<node_type>(point_type::Zero()))
{
    m_thresholdMags.push_back(rootMag);
    m_scales.push_back(rootSize);

    for (std::uint32_t i = 0; i < objects.size(); ++i)
        addObject(i);

    buildIndexMap();
}

template<typename OBJ, typename TRAITS>
std::vector<OBJ>&
OctreeBuilder<OBJ, TRAITS>::objects()
{
    return m_objects;
}

template<typename OBJ, typename TRAITS>
celestia::util::array_view<std::uint32_t>
OctreeBuilder<OBJ, TRAITS>::indices() const
{
    return m_indices;
}

template<typename OBJ, typename TRAITS>
void
OctreeBuilder<OBJ, TRAITS>::addObject(std::uint32_t idx)
{
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

template<typename OBJ, typename TRAITS>
bool
OctreeBuilder<OBJ, TRAITS>::checkStraddling(const point_type& center, std::uint32_t idx)
{
    auto radius = TRAITS::getRadius(m_objects[idx]);
    return radius > 0 && (TRAITS::getPosition(m_objects[idx]) - center).cwiseAbs().minCoeff() < radius;
}

template<typename OBJ, typename TRAITS>
typename OctreeBuilder<OBJ, TRAITS>::node_type*
OctreeBuilder<OBJ, TRAITS>::getChildNode(node_type* node,
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
        child = std::make_unique<node_type>(newCenter, newSize);
        ++m_nodeCount;
    }

    return child.get();
}

template<typename OBJ, typename TRAITS>
void
OctreeBuilder<OBJ, TRAITS>::splitNode(node_type* node, std::uint32_t depth)
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

template<typename OBJ, typename TRAITS>
template<typename VISITOR>
void
OctreeBuilder<OBJ, TRAITS>::processNodes(VISITOR& visitor)
{
    std::vector<std::pair<const node_type*, std::uint32_t>> nodeStack;
    nodeStack.emplace_back(m_root.get(), 0);
    while (!nodeStack.empty())
    {
        auto node = nodeStack.back().first;
        auto depth = nodeStack.back().second;
        nodeStack.pop_back();

        visitor.visit(node, depth);

        if (!node->children)
            continue;

        for (const auto& child : *node->children)
        {
            if (child)
                nodeStack.emplace_back(child.get(), depth + 1);
        }
    }
}

template<typename OBJ, typename TRAITS>
struct OctreeBuilder<OBJ, TRAITS>::IndexMapVisitor
{
    explicit IndexMapVisitor(OctreeBuilder*);

    void visit(const node_type*, std::uint32_t);

    OctreeBuilder* builder;
    std::uint32_t nextIdx{ 0 };
};

template<typename OBJ, typename TRAITS>
OctreeBuilder<OBJ, TRAITS>::IndexMapVisitor::IndexMapVisitor(OctreeBuilder* _builder) :
    builder(_builder)
{
    builder->m_indices.resize(builder->m_objects.size());
}

template<typename OBJ, typename TRAITS>
void
OctreeBuilder<OBJ, TRAITS>::IndexMapVisitor::visit(const node_type* node, std::uint32_t depth)
{
    for (std::uint32_t idx : node->indices)
    {
        builder->m_indices[idx] = nextIdx;
        ++nextIdx;
    }
}

template<typename OBJ, typename TRAITS>
void
OctreeBuilder<OBJ, TRAITS>::buildIndexMap()
{
    IndexMapVisitor visitor(this);
    processNodes(visitor);
}

template<typename OBJ, typename TRAITS>
struct OctreeBuilder<OBJ, TRAITS>::BuildVisitor
{
    explicit BuildVisitor(OctreeBuilder*);

    void visit(const node_type*, std::uint32_t);

    OctreeBuilder* builder;
    std::vector<static_node_type> staticNodes;
    std::vector<std::uint32_t> prevByLevel;
    std::uint32_t topmostPopulated{ std::numeric_limits<std::uint32_t>::max() };
    std::uint32_t maxDepth{ 0 };
};

template<typename OBJ, typename TRAITS>
OctreeBuilder<OBJ, TRAITS>::BuildVisitor::BuildVisitor(OctreeBuilder* _builder) :
    builder(_builder)
{
    staticNodes.reserve(builder->m_nodeCount);
    builder->m_indices.clear();
    builder->m_indices.reserve(builder->m_objects.size());
}

template<typename OBJ, typename TRAITS>
void
OctreeBuilder<OBJ, TRAITS>::BuildVisitor::visit(const node_type* node, std::uint32_t depth)
{
    const auto idx = static_cast<std::uint32_t>(staticNodes.size());

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

    if (depth > maxDepth)
        maxDepth = depth;
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

template<typename OBJ, typename TRAITS>
typename OctreeBuilder<OBJ, TRAITS>::static_octree
OctreeBuilder<OBJ, TRAITS>::build()
{
    BuildVisitor visitor(this);
    processNodes(visitor);
    detail::reorderOctreeElements(m_objects, m_indices);
    return static_octree(std::move(visitor.staticNodes),
                         std::move(m_objects),
                         visitor.topmostPopulated,
                         visitor.maxDepth);
}

} // end namespace celestia::engine
