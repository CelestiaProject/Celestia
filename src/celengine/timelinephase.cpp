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

#include "timelinephase.h"

#include <cassert>

#include <celephem/orbit.h>
#include <celephem/rotation.h>
#include "body.h"
#include "frame.h"
#include "frametree.h"
#include "universe.h"


TimelinePhase::TimelinePhase(CreateToken,
                             Body* body,
                             double startTime,
                             double endTime,
                             const ReferenceFrame::SharedConstPtr& orbitFrame,
                             const std::shared_ptr<const celestia::ephem::Orbit>& orbit,
                             const ReferenceFrame::SharedConstPtr& bodyFrame,
                             const std::shared_ptr<const celestia::ephem::RotationModel>& rotationModel,
                             FrameTree& owner) :
    m_body(body),
    m_startTime(startTime),
    m_endTime(endTime),
    m_orbitFrame(orbitFrame),
    m_orbit(orbit),
    m_bodyFrame(bodyFrame),
    m_rotationModel(rotationModel)
{
    owner.addChild(this);
}

TimelinePhase::~TimelinePhase()
{
    if (m_owner)
        m_owner->removeChild(this);
}

/*! Create a new timeline phase in the specified universe.
 */
std::unique_ptr<TimelinePhase>
TimelinePhase::CreateTimelinePhase(Body* body, //NOSONAR
                                   FrameTree* parent,
                                   double startTime,
                                   double endTime,
                                   const ReferenceFrame::SharedConstPtr& orbitFrame,
                                   const std::shared_ptr<const celestia::ephem::Orbit>& orbit,
                                   const ReferenceFrame::SharedConstPtr& bodyFrame,
                                   const std::shared_ptr<const celestia::ephem::RotationModel>& rotationModel)
{
    // Validate the time range.
    if (!parent || endTime <= startTime)
        return nullptr;

    return std::make_unique<TimelinePhase>(CreateToken(),
                                           body,
                                           startTime,
                                           endTime,
                                           orbitFrame,
                                           orbit,
                                           bodyFrame,
                                           rotationModel,
                                           *parent);
}
