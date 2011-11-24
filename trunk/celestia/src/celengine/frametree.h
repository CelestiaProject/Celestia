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

#include <vector>
#include <cstddef>

class Star;
class Body;
class ReferenceFrame;
class TimelinePhase;


class FrameTree
{
public:
    FrameTree(Star*);
    FrameTree(Body*);
    ~FrameTree();

    /*! Return the star that this tree is associated with; it will be
     *  NULL for frame trees associated with solar system bodies.
     */
    Star* getStar() const
    {
        return starParent;
    }

    ReferenceFrame* getDefaultReferenceFrame() const;

    void addChild(TimelinePhase* phase);
    void removeChild(TimelinePhase* phase);
    TimelinePhase* getChild(unsigned int n) const;
    unsigned int childCount() const;

    void markChanged();
    void markUpdated();
    void recomputeBoundingSphere();

    bool isRoot() const
    {
        return bodyParent == NULL;
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
    std::vector<TimelinePhase*> children;

    double m_boundingSphereRadius;
    double m_maxChildRadius;
    bool m_containsSecondaryIlluminators;
    bool m_changed;
    int m_childClassMask;

    ReferenceFrame* defaultFrame;
};

#endif // _CELENGINE_FRAMETREE_H_
