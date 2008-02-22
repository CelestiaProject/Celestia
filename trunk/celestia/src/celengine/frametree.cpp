// frametree.cpp
//
// Reference frame tree
//
// Copyright (C) 2008, the Celestia Development Team
// Initial version by Chris Laurel, claurel@gmail.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <cassert>
#include "celengine/frametree.h"
#include "celengine/timeline.h"
#include "celengine/timelinephase.h"
#include "celengine/frame.h"


/* A FrameTree is hierarchy of solar system bodies organized according to
 * the relationship of their reference frames. An object will appear in as a child
 * in the tree of whatever object is the center of its orbit frame.
 * Since an object may have several orbit frames in its timeline, the
 * structure is a bit more complicated than a straightforward tree
 * of Body objects. A Body has exactly a single parent in the frame tree
 * at a given time, but may have many over it's lifespan. An object's
 * timeline contains a list of timeline phases; each phase can point to
 * a different parent. Thus, the timeline can be thought of as a list of
 * parents.
 */

/*! Create a frame tree associated with a star.
 */
FrameTree::FrameTree(Star* star) :
    starParent(star),
    bodyParent(NULL),
    defaultFrame(NULL)
{
    // Default frame for a star is J2000 ecliptical, centered
    // on the star.
    defaultFrame = new J2000EclipticFrame(Selection(star));
    defaultFrame->addRef();
}


/*! Create a frame tree associated with a planet or other solar system body.
 */
FrameTree::FrameTree(Body* body) :
    starParent(NULL),
    bodyParent(body),
    defaultFrame(NULL)
{
    // Default frame for a solar system body is the mean equatorial frame of the body.
    defaultFrame = new BodyMeanEquatorFrame(Selection(body), Selection(body));
    defaultFrame->addRef();
}


FrameTree::~FrameTree()
{
    defaultFrame->release();
}


/*! Return the default reference frame for the object a frame tree is associated
 *  with.
 */
ReferenceFrame*
FrameTree::getDefaultReferenceFrame() const
{
    return defaultFrame;
}


/*! Mark this node of the frame hierarchy as changed. The changed flag
 *  is propagated up toward the root of the tree.
 */
void
FrameTree::markChanged()
{
    if (!changed)
    {
        changed = true;
        if (bodyParent != NULL)
            bodyParent->markChanged();
    }
}


/*! Mark this node of the frame hierarchy as updated. The changed flag
 *  is marked false in this node and in all child nodes that
 *  were marked changed.
 */
void
FrameTree::markUpdated()
{
    if (changed)
    {
        changed = false;
        for (vector<TimelinePhase*>::iterator iter = children.begin();
             iter != children.end(); iter++)
        {
            (*iter)->body()->markUpdated();
        }
    }
}


/*! Recompute the bounding sphere for this tree and all subtrees marked
 *  as having changed. The bounding sphere is large enough to accommodate
 *  the orbits (and radii) of all child bodies.
 */
void
FrameTree::recomputeBoundingSphere()
{
    if (changed)
    {
        boundingSphereRadius = 0.0;
        for (vector<TimelinePhase*>::iterator iter = children.begin();
             iter != children.end(); iter++)
        {
            TimelinePhase* phase = *iter;
            double r = phase->body()->getRadius() + phase->orbit()->getBoundingRadius();

            FrameTree* tree = phase->body()->getFrameTree();
            if (tree != NULL)
            {
                tree->recomputeBoundingSphere();
                r += tree->boundingSphereRadius;
            }

            boundingSphereRadius = max(boundingSphereRadius, r);
        }
    }
}


/*! Add a new phase to this tree.
 */
void
FrameTree::addChild(TimelinePhase* phase)
{
    phase->addRef();
    children.push_back(phase);
    markChanged();
}


/*! Remove a phase from the tree. This method does nothing if the specified
 *  phase doesn't exist in the tree.
 */
void
FrameTree::removeChild(TimelinePhase* phase)
{
    vector<TimelinePhase*>::iterator iter = find(children.begin(), children.end(), phase);
    if (iter != children.end())
    {
        (*iter)->release();
        children.erase(iter);
        markChanged();
    }
}


/*! Return the child at the specified index. */
TimelinePhase*
FrameTree::getChild(unsigned int n) const
{
    return children[n];
}


/*! Get the number of immediate children of this tree. */
unsigned int
FrameTree::childCount() const
{
    return children.size();
}
