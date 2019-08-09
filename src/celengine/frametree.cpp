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
#include <celengine/body.h>
#include <celengine/star.h>
#include <celengine/location.h>
#include <celengine/deepskyobj.h>

/* A FrameTree is hierarchy of solar system bodies organized according to
 * the relationship of their reference frames. An object will appear in as
 * a child in the tree of whatever object is the center of its orbit frame.
 * Since an object may have several orbit frames in its timeline, the
 * structure is a bit more complicated than a straightforward tree
 * of Body objects. A Body has exactly a single parent in the frame tree
 * at a given time, but may have many over it's lifespan. An object's
 * timeline contains a list of timeline phases; each phase can point to
 * a different parent. Thus, the timeline can be thought of as a list of
 * parents.
 *
 * The FrameTree hiearchy is designed for fast visibility culling. There are
 * two values stored in each node for this purpose: the bounding sphere
 * radius, and the maximum child object radius. The bounding sphere is large
 * enough to contain the orbits of all child objects, as well as the child
 * objects themselves. Change tracking is performed whenever the frame tree
 * is modified: adding a node, removing a node, or changing the radius of an
 * object will all cause the tree to be marked as changed.
 */

using namespace std;

/*! Create a frame tree associated with a star.
 */
FrameTree::FrameTree(Star* star) :
    starParent(star),
    bodyParent(nullptr),
    m_changed(true),
    defaultFrame(nullptr)
{
    // Default frame for a star is J2000 ecliptical, centered
    // on the star.
    defaultFrame = make_shared<J2000EclipticFrame>(Selection(star));
}


/*! Create a frame tree associated with a planet or other solar system body.
 */
FrameTree::FrameTree(Body* body) :
    starParent(nullptr),
    bodyParent(body),
    m_changed(true),
    defaultFrame(nullptr)
{
    // Default frame for a solar system body is the mean equatorial frame of the body.
    defaultFrame = make_shared<BodyMeanEquatorFrame>(Selection(body), Selection(body));
}


FrameTree::~FrameTree()
{
}


/*! Return the default reference frame for the object a frame tree is associated
 *  with.
 */
const shared_ptr<const ReferenceFrame>&
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
    if (!m_changed)
    {
        m_changed = true;
        if (bodyParent != nullptr)
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
    if (m_changed)
    {
        m_changed = false;
        for (const auto child : children)
            child->body()->markUpdated();
    }
}


/*! Recompute the bounding sphere for this tree and all subtrees marked
 *  as having changed. The bounding sphere is large enough to accommodate
 *  the orbits (and radii) of all child bodies. This method also recomputes
 *  the maximum child radius, secondary illuminator status, and child
 *  class mask.
 */
void
FrameTree::recomputeBoundingSphere()
{
    if (m_changed)
    {
        m_boundingSphereRadius = 0.0;
        m_maxChildRadius = 0.0;
        m_containsSecondaryIlluminators = false;
        m_childClassMask = 0;

        for (const auto phase : children)
        {
            double bodyRadius = phase->body()->getRadius();
            double r = phase->body()->getCullingRadius() + phase->orbit()->getBoundingRadius();
            m_maxChildRadius = max(m_maxChildRadius, bodyRadius);
            m_containsSecondaryIlluminators = m_containsSecondaryIlluminators || phase->body()->isSecondaryIlluminator();
            m_childClassMask |= phase->body()->getClassification();

            FrameTree* tree = phase->body()->getFrameTree();
            if (tree != nullptr)
            {
                tree->recomputeBoundingSphere();
                r += tree->m_boundingSphereRadius;
                m_maxChildRadius = max(m_maxChildRadius, tree->m_maxChildRadius);
                m_containsSecondaryIlluminators = m_containsSecondaryIlluminators || tree->containsSecondaryIlluminators();
                m_childClassMask |= tree->childClassMask();
            }

            m_boundingSphereRadius = max(m_boundingSphereRadius, r);
        }
    }
}


/*! Add a new phase to this tree.
 */
void
FrameTree::addChild(shared_ptr<const TimelinePhase> &phase)
{
    children.push_back(phase);
    markChanged();
}


/*! Remove a phase from the tree. This method does nothing if the specified
 *  phase doesn't exist in the tree.
 */
void
FrameTree::removeChild(shared_ptr<const TimelinePhase> &phase)
{
    auto iter = find(children.begin(), children.end(), phase);
    if (iter != children.end())
    {
        children.erase(iter);
        markChanged();
    }
}


/*! Return the child at the specified index. */
const shared_ptr<const TimelinePhase>&
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
