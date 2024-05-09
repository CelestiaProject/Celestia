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
#include <tuple>
#include <utility>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <celengine/observer.h>

// There are two classes implemented in this module: StaticOctree and
// DynamicOctree.  The DynamicOctree is built first by inserting
// objects from a database or catalog and is then 'compiled' into a StaticOctree.
// In the process of building the StaticOctree, the original object database is
// reorganized, with objects in the same octree node all placed adjacent to each
// other.  This spatial sorting of the objects dramatically improves the
// performance of octree operations through much more coherent memory access.

// The DynamicOctree and StaticOctree template arguments are:
// OBJ:    object hanging from the node
// TRAITS: dynamic octree trait methods
// PREC:   floating point precision of the culling operations at node level.
// The hierarchy of octree nodes is built using a single precision value (excludingFactor), which relates to an
// OBJ's limiting property defined by the octree particular specialization: ie. we use [absolute magnitude] for star octrees, etc.
// For details, see notes below.

// The SPLIT_THRESHOLD is the number of objects a node must contain before its
// children are generated. Increasing this number will decrease the number of
// octree nodes in the tree, which will use less memory but make culling less
// efficient.

template<class OBJ, class PREC>
class OctreeProcessor
{
 public:
    OctreeProcessor()          {};
    virtual ~OctreeProcessor() {};

    virtual void process(const OBJ& obj, PREC distance, float appMag) = 0;
};

struct OctreeLevelStatistics
{
    std::uint32_t nodeCount{ 0 };
    std::uint32_t objectCount{ 0 };
};

template<class OBJ, class TRAITS> class StaticOctree;

namespace detail
{

template<class OBJ, class PREC>
struct DynamicOctreeNode
{
    using child_array = std::array<std::unique_ptr<DynamicOctreeNode>, 8>;
    using point_type = Eigen::Matrix<PREC, 3, 1>;

    explicit DynamicOctreeNode(const point_type&);
    ~DynamicOctreeNode();

    point_type center;
    std::vector<const OBJ*> objects;
    std::unique_ptr<child_array> children;
};

template<class OBJ, class PREC>
DynamicOctreeNode<OBJ, PREC>::DynamicOctreeNode(const point_type& _center) :
    center(_center)
{
}

template<class OBJ, class PREC>
DynamicOctreeNode<OBJ, PREC>::~DynamicOctreeNode() = default;

} // end namespace detail

template<class OBJ, class TRAITS>
class DynamicOctree
{
public:
    using precision_type = typename decltype(TRAITS::getPosition(std::declval<const OBJ&>()))::Scalar;
    using point_type = Eigen::Matrix<precision_type, 3, 1>;
    using static_octree_type = StaticOctree<OBJ, TRAITS>;

    DynamicOctree(const point_type& center,
                  precision_type scale,
                  float thresholdMag);

    void insertObject(const OBJ&);

    std::unique_ptr<static_octree_type> rebuildAndSort(OBJ*&);

private:
    using node_type = detail::DynamicOctreeNode<OBJ, precision_type>;

    bool checkStraddling(const point_type&, const OBJ&);
    node_type* getChildNode(node_type*, const OBJ&, std::uint32_t);
    void splitNode(node_type*, std::uint32_t);

    std::vector<float> m_thresholdMags;
    std::vector<precision_type> m_scales;
    std::unique_ptr<node_type> m_root;
};

template<class OBJ, class TRAITS>
DynamicOctree<OBJ, TRAITS>::DynamicOctree(const point_type& center,
                                          precision_type scale,
                                          float thresholdMag) :
    m_root(std::make_unique<node_type>(center))
{
    m_thresholdMags.push_back(thresholdMag);
    m_scales.push_back(scale);
}

template<class OBJ, class TRAITS>
void
DynamicOctree<OBJ, TRAITS>::insertObject(const OBJ& obj)
{
    std::uint32_t depth = 0;
    auto node = m_root.get();
    for (;;)
    {
        if (depth >= m_thresholdMags.size())
            m_thresholdMags.push_back(TRAITS::decayMagnitude(m_thresholdMags.back()));

        float thresholdMag = m_thresholdMags[depth];

        if (checkStraddling(node->center, obj) || TRAITS::getAbsMag(obj) <= thresholdMag)
        {
            node->objects.push_back(&obj);
            return;
        }

        ++depth;

        // If we haven't allocated child nodes yet, try to fit
        // the object in this node, even though it could be put
        // in a child. Only if there are more than SPLIT_THRESHOLD
        // objects in the node will we attempt to place the
        // object into a child node.  This is done in order
        // to avoid having the octree degenerate into one object
        // per node.
        if (!node->children)
        {
            if (node->objects.size() < TRAITS::SplitThreshold)
            {
                node->objects.push_back(&obj);
                return;
            }

            // We've run out of space: split the node
            splitNode(node, depth);
        }

        node = getChildNode(node, obj, depth);
    }
}

template<class OBJ, class TRAITS>
bool
DynamicOctree<OBJ, TRAITS>::checkStraddling(const point_type& center, const OBJ& obj)
{
    auto radius = TRAITS::getRadius(obj);
    return radius > 0 && (TRAITS::getPosition(obj) - center).cwiseAbs().minCoeff() < radius;
}

template<class OBJ, class TRAITS>
typename DynamicOctree<OBJ, TRAITS>::node_type*
DynamicOctree<OBJ, TRAITS>::getChildNode(node_type* node, const OBJ& obj, std::uint32_t depth)
{
    auto position = TRAITS::getPosition(obj);
    int index = static_cast<int>(position.x() >= node->center.x())
              | (static_cast<int>(position.y() >= node->center.y()) << 1)
              | (static_cast<int>(position.z() >= node->center.z()) << 2);
    auto& child = (*node->children)[index];
    if (!child)
    {
        if (depth >= m_scales.size())
            m_scales.push_back(m_scales.back() * precision_type(0.5));

        auto scale = m_scales[depth];
        auto newCenter = node->center
                       + scale * point_type(((index & 1) << 1) - 1,
                                            (index & 2) - 1,
                                            ((index & 4) >> 1) - 1);
        child = std::make_unique<node_type>(newCenter);
    }

    return child.get();
}

template<class OBJ, class TRAITS>
void
DynamicOctree<OBJ, TRAITS>::splitNode(node_type* node, std::uint32_t depth)
{
    const float thresholdMag = m_thresholdMags[depth];
    node->children = std::make_unique<typename node_type::child_array>();
    const auto end = node->objects.end();
    auto writeIt = node->objects.begin();
    for (auto readIt = node->objects.begin(); readIt != end; ++readIt)
    {
        if (checkStraddling(node->center, **readIt) || TRAITS::getAbsMag(**readIt) <= thresholdMag)
        {
            *writeIt = *readIt;
            ++writeIt;
            continue;
        }

        getChildNode(node, **readIt, depth)->objects.push_back(*readIt);
    }

    node->objects.erase(writeIt, end);
}

template <class OBJ, class TRAITS>
class StaticOctree
{
public:
    using precision_type = typename decltype(TRAITS::getPosition(std::declval<const OBJ&>()))::Scalar;
    using point_type = Eigen::Matrix<precision_type, 3, 1>;
    using processor_type = OctreeProcessor<OBJ, precision_type>;
    using plane_type = Eigen::Hyperplane<precision_type, 3>;

    StaticOctree(const point_type& cellCenterPos,
                 float exclusionFactor,
                 OBJ* firstObject,
                 std::uint32_t nObjects);
    ~StaticOctree();

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
    void processVisibleObjects(processor_type& processor,
                               const point_type& obsPosition,
                               const plane_type* frustumPlanes,
                               float limitingFactor,
                               precision_type scale) const;

    void processCloseObjects(processor_type& processor,
                             const point_type& obsPosition,
                             precision_type boundingRadius,
                             precision_type scale) const;

    std::uint32_t countChildren() const;
    std::uint32_t countObjects()  const;

    void computeStatistics(std::vector<OctreeLevelStatistics>& stats, std::uint32_t level = 0);

private:
    using child_array = std::array<std::unique_ptr<StaticOctree>, 8>;

    std::unique_ptr<child_array> _children;
    point_type _cellCenterPos;
    float _exclusionFactor;
    OBJ* _firstObject;
    std::uint32_t _nObjects;

    friend class DynamicOctree<OBJ, TRAITS>;
};

template <class OBJ, class TRAITS>
std::unique_ptr<typename DynamicOctree<OBJ, TRAITS>::static_octree_type>
DynamicOctree<OBJ, TRAITS>::rebuildAndSort(OBJ*& _sortedObjects)
{
    std::unique_ptr<static_octree_type> root;
    std::vector<std::tuple<std::unique_ptr<static_octree_type>*, const node_type*, std::uint32_t>> processStack;
    processStack.emplace_back(&root, m_root.get(), 0);
    while (!processStack.empty())
    {
        auto entry = std::move(processStack.back());
        processStack.pop_back();

        auto [staticNode, dynamicNode, depth] = entry;

        OBJ* firstObject = _sortedObjects;
        for (const OBJ* obj : dynamicNode->objects)
        {
            *_sortedObjects = *obj;
            ++_sortedObjects;
        }

        auto nObjects = static_cast<std::uint32_t>(_sortedObjects - firstObject);
        *staticNode = std::make_unique<static_octree_type>(dynamicNode->center, m_thresholdMags[depth], firstObject, nObjects);

        if (!dynamicNode->children)
        {
            processStack.pop_back();
            continue;
        }

        (*staticNode)->_children = std::make_unique<static_octree_type::child_array>();
        for (unsigned int i = 8; i-- > 0;)
        {
            auto dynamicChild = (*dynamicNode->children)[i].get();
            if (!dynamicChild)
                continue;

            auto staticChild = (*staticNode)->_children->data() + i;
            processStack.emplace_back(staticChild, dynamicChild, depth + 1);
        }
    }

    return root;
}

template <class OBJ, class TRAITS>
StaticOctree<OBJ, TRAITS>::StaticOctree(const point_type& cellCenterPos,
                                        const float exclusionFactor,
                                        OBJ* firstObject,
                                        std::uint32_t nObjects) :
    _cellCenterPos(cellCenterPos),
    _exclusionFactor(exclusionFactor),
    _firstObject(firstObject),
    _nObjects(nObjects)
{
}

template <class OBJ, class TRAITS>
inline StaticOctree<OBJ, TRAITS>::~StaticOctree() = default;

template <class OBJ, class TRAITS>
std::uint32_t
StaticOctree<OBJ, TRAITS>::countChildren() const
{
    std::uint32_t count = 0;
    std::vector<const StaticOctree*> nodeStack;
    nodeStack.push_back(this);
    while (!nodeStack.empty())
    {
        const StaticOctree* node = nodeStack.back();
        nodeStack.pop_back();

        ++count;

        if (!node->_children)
            continue;

        for (const auto& child : *_children)
        {
            if (child)
                nodeStack.push_back(child.get());
        }
    }

    return count;
}

template <class OBJ, class TRAITS>
std::uint32_t
StaticOctree<OBJ, TRAITS>::countObjects() const
{
    std::uint32_t count = 0;
    std::vector<const StaticOctree*> nodeStack;
    nodeStack.push_back(this);
    while (!nodeStack.empty())
    {
        const StaticOctree* node = nodeStack.back();
        nodeStack.pop_back();

        count += node->_nObjects;

        if (!node->_children)
            continue;

        for (const auto& child : *_children)
        {
            if (child)
                nodeStack.push_back(child.get());
        }
    }

    return count;
}

template <class OBJ, class PREC>
void StaticOctree<OBJ, PREC>::computeStatistics(std::vector<OctreeLevelStatistics>& stats, std::uint32_t level)
{
    while (level >= stats.size())
        stats.emplace_back();

    stats[level].nodeCount++;
    stats[level].objectCount += _nObjects;

    if (_children != nullptr)
    {
        for (int i = 0; i < 8; i++)
            _children[i]->computeStatistics(stats, level + 1);
    }
}
