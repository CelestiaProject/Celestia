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
#include "selection.h"

class ReferenceFrame;
class Star;
class TimelinePhase;

class FrameTree //NOSONAR
{
public:
    explicit FrameTree(Star*);
    explicit FrameTree(Body*);
    ~FrameTree();

    FrameTree(const FrameTree&) = delete;
    FrameTree& operator=(const FrameTree&) = delete;
    FrameTree(FrameTree&&) = delete;
    FrameTree& operator=(FrameTree&&) = delete;

    /*! Return the object that this tree is associated with.
     */
    Selection getOwner() const noexcept { return m_owner; }
    Star* getRoot(double tjd) const;

    const TimelinePhase* getChild(unsigned int n) const;
    unsigned int childCount() const;

    void markChanged();
    void markUpdated();
    void recomputeBoundingSphere();

    bool updateRequired() const noexcept { return m_changed; }

    /*! Get the radius of a sphere large enough to contain all
     *  objects in the tree.
     */
    double boundingSphereRadius() const noexcept { return m_boundingSphereRadius; }

    /*! Get the radius of the largest body in the tree.
     */
    double maxChildRadius() const noexcept { return m_maxChildRadius; }

    /*! Return whether any of the children of this frame
     *  are secondary illuminators.
     */
    bool containsSecondaryIlluminators() const noexcept { return m_containsSecondaryIlluminators; }

    /*! Return a bitmask with the classifications of all children
     *  in this tree.
     */
    BodyClassification childClassMask() const noexcept { return m_childClassMask; }

private:
    void addChild(TimelinePhase* phase);
    void removeChild(TimelinePhase* phase);

    Selection m_owner;
    std::vector<TimelinePhase*> m_children;

    double m_boundingSphereRadius{ 0.0 };
    double m_maxChildRadius{ 0.0 };
    bool m_containsSecondaryIlluminators{ false };
    bool m_changed{ true };
    BodyClassification m_childClassMask{ BodyClassification::EmptyMask };

    friend class TimelinePhase;
};
