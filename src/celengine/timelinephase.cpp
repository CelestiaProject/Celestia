// timelinephase.cpp
//
// Object timeline phase
//
// Copyright (C) 2008, the Celestia Development Team
// Initial version by Chris Laurel, claurel@gmail.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include "celengine/timelinephase.h"
#include "celengine/frame.h"
#include "celengine/orbit.h"
#include "celengine/rotation.h"
#include "celengine/universe.h"
#include "celengine/frametree.h"


TimelinePhase::TimelinePhase(Body* _body,
                             double _startTime,
                             double _endTime,
                             ReferenceFrame* _orbitFrame,
                             Orbit* _orbit,
                             ReferenceFrame* _bodyFrame,
                             RotationModel* _rotationModel,
                             FrameTree* _owner) :
    m_body(_body),
    m_startTime(_startTime),
    m_endTime(_endTime),
    m_orbitFrame(_orbitFrame),
    m_orbit(_orbit),
    m_bodyFrame(_bodyFrame),
    m_rotationModel(_rotationModel),
    m_owner(_owner),
    refCount(0)
{
    // assert(owner == orbitFrame->getCenter()->getFrameTree());
    m_orbitFrame->addRef();
    m_bodyFrame->addRef();
}


TimelinePhase::~TimelinePhase()
{
    m_orbitFrame->release();
    m_bodyFrame->release();
}


// Declared private--should never be used
TimelinePhase::TimelinePhase(const TimelinePhase&)
{
    assert(0);
}


// Declared private--should never be used
TimelinePhase& TimelinePhase::operator=(const TimelinePhase&)
{
    assert(0);
    return *this;
}


int TimelinePhase::addRef() const
{
    return ++refCount;
}


int TimelinePhase::release() const
{
    --refCount;
    assert(refCount >= 0);
    if (refCount <= 0)
        delete this;

    return refCount;
}


/*! Create a new timeline phase in the specified universe.
 */
TimelinePhase*
TimelinePhase::CreateTimelinePhase(Universe& universe,
                                   Body* body,
                                   double startTime,
                                   double endTime,
                                   ReferenceFrame& orbitFrame,
                                   Orbit& orbit,
                                   ReferenceFrame& bodyFrame,
                                   RotationModel& rotationModel)
{
    // Validate the time range.
    if (endTime <= startTime)
        return NULL;

    // Get the frame tree to add the new phase to. Verify that the reference frame
    // center is either a star or solar system body.
    FrameTree* frameTree = NULL;
    Selection center = orbitFrame.getCenter();
    if (center.body() != NULL)
    {
        frameTree = center.body()->getOrCreateFrameTree();
    }
    else if (center.star() != NULL)
    {
        SolarSystem* solarSystem = universe.getSolarSystem(center.star());
        if (solarSystem == NULL)
        {
            // No solar system defined for this star yet, so we need
            // to create it.
            solarSystem = universe.createSolarSystem(center.star());
        }
        frameTree = solarSystem->getFrameTree();
    }
    else
    {
        // Frame center is not a star or body.
        return NULL;
    }

    TimelinePhase* phase = new TimelinePhase(body,
                                             startTime,
                                             endTime,
                                             &orbitFrame,
                                             &orbit,
                                             &bodyFrame,
                                             &rotationModel,
                                             frameTree);

    frameTree->addChild(phase);

    return phase;
}
