// frametree.h
//
// Reference frame tree.
//
// Copyright (C) 2008, the Celestia Development Team
// Initial version by Chris Laurel, claurel@gmail.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>
#include <vector>

#include "body.h"

class ReferenceFrame;
class Star;
class TimelinePhase;

class FrameTree //NOSONAR
{
public:
    explicit FrameTree(Star*);
    explicit FrameTree(Body*);
    ~FrameTree();

    /*! Return the star that this tree is associated with; it will be
     *  nullptr for frame trees associated with solar system bodies.
     */
    Star* getStar() const
    {
        return starParent;
    }

    const std::shared_ptr<const ReferenceFrame>& getDefaultReferenceFrame() const;

    void addChild(const std::shared_ptr<const TimelinePhase>& phase);
    void removeChild(const std::shared_ptr<const TimelinePhase>& phase);
    const TimelinePhase* getChild(unsigned int n) const;
    unsigned int childCount() const;

    void markChanged();
    void markUpdated();
    void recomputeBoundingSphere();

    bool isRoot() const
    {
        return bodyParent == nullptr;
    }

    bool updateRequired() const
    {
        return m_changed;
    }

    /*! Get the radius of a sphere large enough to contain all
     *  objects in the tree.
     */
    double boundingSphereRadius() const
    {
        return m_boundingSphereRadius;
    }

    /*! Get the radius of the largest body in the tree.
     */
    double maxChildRadius() const
    {
        return m_maxChildRadius;
    }

    /*! Return whether any of the children of this frame
     *  are secondary illuminators.
     */
    bool containsSecondaryIlluminators() const
    {
        return m_containsSecondaryIlluminators;
    }

    /*! Return a bitmask with the classifications of all children
     *  in this tree.
     */
    BodyClassification childClassMask() const
    {
        return m_childClassMask;
    }

private:
    Star* starParent{ nullptr };
    Body* bodyParent{ nullptr };
    std::vector<std::shared_ptr<const TimelinePhase>> children;

    double m_boundingSphereRadius{ 0.0 };
    double m_maxChildRadius{ 0.0 };
    bool m_containsSecondaryIlluminators{ false };
    bool m_changed{ true };
    BodyClassification m_childClassMask{ BodyClassification::EmptyMask };

    std::shared_ptr<const ReferenceFrame> defaultFrame;
};
