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

#ifndef _CELENGINE_FRAMETREE_H_
#define _CELENGINE_FRAMETREE_H_

#include <memory>
#include <vector>
#include <cstddef>
#include "frame.h"
#include "timelinephase.h"

class Star;
class Body;

class FrameTree
{
public:
    FrameTree(Star*);
    FrameTree(Body*);
    ~FrameTree() = default;

    /*! Return the star that this tree is associated with; it will be
     *  nullptr for frame trees associated with solar system bodies.
     */
    Star* getStar() const
    {
        return starParent;
    }

    const ReferenceFrame::SharedConstPtr &getDefaultReferenceFrame() const;

    void addChild(const TimelinePhase::SharedConstPtr &phase);
    void removeChild(const TimelinePhase::SharedConstPtr &phase);
    const TimelinePhase::SharedConstPtr &getChild(unsigned int n) const;
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
    int childClassMask() const
    {
        return m_childClassMask;
    }

private:
    Star* starParent;
    Body* bodyParent;
    std::vector<TimelinePhase::SharedConstPtr> children;

    double m_boundingSphereRadius{ 0.0 };
    double m_maxChildRadius{ 0.0 };
    bool m_containsSecondaryIlluminators{ false };
    bool m_changed{ false };
    int m_childClassMask{ 0 };

    ReferenceFrame::SharedConstPtr defaultFrame;
};

#endif // _CELENGINE_FRAMETREE_H_
