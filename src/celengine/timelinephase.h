// timelinephase.h
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

#pragma once

#include <memory>
#include "frame.h"

class FrameTree;
class Universe;
class Body;

namespace celestia::ephem
{
class Orbit;
class RotationModel;
}

class TimelinePhase //NOSONAR
{
private:
    struct CreateToken {};

public:
    TimelinePhase(CreateToken,
                  Body*,
                  double,
                  double,
                  const ReferenceFrame::SharedConstPtr&,
                  const std::shared_ptr<const celestia::ephem::Orbit>&,
                  const ReferenceFrame::SharedConstPtr&,
                  const std::shared_ptr<const celestia::ephem::RotationModel>&,
                  FrameTree*);
    ~TimelinePhase();

    TimelinePhase(const TimelinePhase& phase) = delete;
    TimelinePhase& operator=(const TimelinePhase& phase) = delete;
    TimelinePhase(TimelinePhase&&) = delete;
    TimelinePhase& operator=(TimelinePhase&&) = delete;

    Body* body() const
    {
        return m_body;
    }

    double startTime() const
    {
        return m_startTime;
    }

    double endTime() const
    {
        return m_endTime;
    }

    const ReferenceFrame::SharedConstPtr& orbitFrame() const
    {
        return m_orbitFrame;
    }

    const std::shared_ptr<const celestia::ephem::Orbit>& orbit() const
    {
        return m_orbit;
    }

    const ReferenceFrame::SharedConstPtr& bodyFrame() const
    {
        return m_bodyFrame;
    }

    const std::shared_ptr<const celestia::ephem::RotationModel>& rotationModel() const
    {
        return m_rotationModel;
    }

    /*! Get the frame tree that contains this phase (always the tree associated
     *  with the center of the orbit frame.)
     */
    FrameTree* getFrameTree() const
    {
        return m_owner;
    }

    /*! Check whether the specified time t lies within this phase. True if
     *  startTime <= t < endTime
     */
    bool includes(double t) const
    {
        return m_startTime <= t && t < m_endTime;
    }

    static std::unique_ptr<TimelinePhase>
    CreateTimelinePhase(Universe& universe,
                        Body* body,
                        double startTime,
                        double endTime,
                        const ReferenceFrame::SharedConstPtr& orbitFrame,
                        const std::shared_ptr<const celestia::ephem::Orbit>& orbit,
                        const ReferenceFrame::SharedConstPtr& bodyFrame,
                        const std::shared_ptr<const celestia::ephem::RotationModel>& rotationModel);

private:
    Body* m_body;

    double m_startTime;
    double m_endTime;

    ReferenceFrame::SharedConstPtr m_orbitFrame;
    std::shared_ptr<const celestia::ephem::Orbit> m_orbit;
    ReferenceFrame::SharedConstPtr m_bodyFrame;
    std::shared_ptr<const celestia::ephem::RotationModel> m_rotationModel;

    FrameTree* m_owner{ nullptr };

    friend class FrameTree;
};
