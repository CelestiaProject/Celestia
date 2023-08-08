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
#include <optional>
#include "frame.h"

class FrameTree;
class Universe;
class Body;

namespace celestia::ephem
{
class Orbit;
class RotationModel;
}


class TimelinePhase
{
public:
    using SharedPtr = std::shared_ptr<TimelinePhase>;
    using SharedConstPtr = std::shared_ptr<const TimelinePhase>;
    Body* body() const
    {
        return m_body;
    }

    std::optional<double> startTime() const
    {
        return m_startTime;
    }

    std::optional<double> endTime() const
    {
        return m_endTime;
    }

    const ReferenceFrame::SharedConstPtr& orbitFrame() const
    {
        return m_orbitFrame;
    }

    celestia::ephem::Orbit* orbit() const
    {
        return m_orbit;
    }

    const ReferenceFrame::SharedConstPtr& bodyFrame() const
    {
        return m_bodyFrame;
    }

    celestia::ephem::RotationModel* rotationModel() const
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
        if (m_startTime.has_value() && t < m_startTime.value())
            return false;
        if (m_endTime.has_value() && t >= m_endTime.value())
            return false;
        return true;
    }

    double clamp(double t) const
    {
        if (m_startTime.has_value() && t < m_startTime.value())
            return m_startTime.value();
        if (m_endTime.has_value() && t > m_endTime.value())
            return m_endTime.value();
        return t;
    }

    static TimelinePhase::SharedConstPtr CreateTimelinePhase(Universe& universe,
                                                             Body* body,
                                                             std::optional<double> startTime,
                                                             std::optional<double> endTime,
                                                             const ReferenceFrame::SharedConstPtr& orbitFrame,
                                                             celestia::ephem::Orbit& orbit,
                                                             const ReferenceFrame::SharedConstPtr& bodyFrame,
                                                             celestia::ephem::RotationModel& rotationModel);

    ~TimelinePhase() = default;

    TimelinePhase(Body* _body,
                  std::optional<double> _startTime,
                  std::optional<double> _endTime,
                  const ReferenceFrame::SharedConstPtr& _orbitFrame,
                  celestia::ephem::Orbit* _orbit,
                  const ReferenceFrame::SharedConstPtr& _bodyFrame,
                  celestia::ephem::RotationModel* _rotationModel,
                  FrameTree* _owner);

    TimelinePhase(const TimelinePhase& phase) = delete;
    TimelinePhase& operator=(const TimelinePhase& phase) = delete;

private:
    Body* m_body;

    std::optional<double> m_startTime;
    std::optional<double> m_endTime;

    ReferenceFrame::SharedConstPtr m_orbitFrame;
    celestia::ephem::Orbit* m_orbit;
    ReferenceFrame::SharedConstPtr m_bodyFrame;
    celestia::ephem::RotationModel* m_rotationModel;

    FrameTree* m_owner;
};
