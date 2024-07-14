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

#include <Eigen/Core>
#include <Eigen/Geometry>

// The StaticOctree template arguments are:
// OBJ:  object hanging from the node,
// PREC: floating point precision of the culling operations at node level.
// The hierarchy of octree nodes is built using a single precision value (excludingFactor), which relates to an
// OBJ's limiting property defined by the octree particular specialization: ie. we use [absolute magnitude] for star octrees, etc.
// For details, see notes below.

template <class OBJ, class PREC>
class OctreeProcessor
{
public:
    virtual ~OctreeProcessor() = default;
    virtual void process(const OBJ& obj, PREC distance, float appMag) = 0;

protected:
    OctreeProcessor() = default;
};

template<class OBJ, class PREC>
class DynamicOctree;

template<class OBJ, class PREC>
class StaticOctree
{
public:
    using PointType = Eigen::Matrix<PREC, 3, 1>;
    using PlaneType = Eigen::Hyperplane<PREC, 3>;

    StaticOctree(const PointType& cellCenterPos,
                 const float      exclusionFactor,
                 OBJ*             _firstObject,
                 std::uint32_t    nObjects);

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
    void processVisibleObjects(OctreeProcessor<OBJ, PREC>& processor,
                               const PointType&            obsPosition,
                               const PlaneType*            frustumPlanes,
                               float                       limitingFactor,
                               PREC                        scale) const;

    void processCloseObjects(OctreeProcessor<OBJ, PREC>& processor,
                             const PointType&            obsPosition,
                             PREC                        boundingRadius,
                             PREC                        scale) const;

    std::uint32_t countChildren() const;
    std::uint32_t countObjects()  const;

private:
    using ChildrenType = std::array<std::unique_ptr<StaticOctree>, 8>;

    std::unique_ptr<ChildrenType> m_children;
    PointType m_cellCenterPos;
    float m_exclusionFactor;
    std::uint32_t m_nObjects;
    OBJ* m_firstObject;

    friend class DynamicOctree<OBJ, PREC>;
};

template<class OBJ, class PREC>
StaticOctree<OBJ, PREC>::StaticOctree(const PointType& cellCenterPos,
                                      float exclusionFactor,
                                      OBJ* firstObject,
                                      std::uint32_t nObjects):
    m_cellCenterPos(cellCenterPos),
    m_exclusionFactor(exclusionFactor),
    m_firstObject(firstObject),
    m_nObjects(nObjects)
{
}

template<class OBJ, class PREC>
std::uint32_t
StaticOctree<OBJ, PREC>::countChildren() const
{
    if (m_children == nullptr)
        return 0;

    std::uint32_t count    = 0;

    for (int i = 0; i < 8; ++i)
        count += UINT32_C(1) + (*m_children)[i]->countChildren();

    return count;
}

template<class OBJ, class PREC>
std::uint32_t
StaticOctree<OBJ, PREC>::countObjects() const
{
    std::uint32_t count = m_nObjects;

    if (m_children != nullptr)
        for (int i = 0; i < 8; ++i)
            count += (*m_children)[i]->countObjects();

    return count;
}
