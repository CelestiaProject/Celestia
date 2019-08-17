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

#ifndef _CELENGINE_TIMELINEPHASE_H_
#define _CELENGINE_TIMELINEPHASE_H_

#include <memory>
#include "frame.h"

class Orbit;
class RotationModel;
class FrameTree;
class Universe;
class Body;


class TimelinePhase
{
public:
    using SharedPtr = std::shared_ptr<TimelinePhase>;
    using SharedConstPtr = std::shared_ptr<const TimelinePhase>;
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

    Orbit* orbit() const
    {
        return m_orbit;
    }

    const ReferenceFrame::SharedConstPtr& bodyFrame() const
    {
        return m_bodyFrame;
    }

    RotationModel* rotationModel() const
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

    static TimelinePhase::SharedConstPtr CreateTimelinePhase(Universe& universe,
                                                             Body* body,
                                                             double startTime,
                                                             double endTime,
                                                             const ReferenceFrame::SharedConstPtr& orbitFrame,
                                                             Orbit& orbit,
                                                             const ReferenceFrame::SharedConstPtr& bodyFrame,
                                                             RotationModel& rotationModel);

    ~TimelinePhase() = default;

    TimelinePhase(Body* _body,
                  double _startTime,
                  double _endTime,
                  const ReferenceFrame::SharedConstPtr& _orbitFrame,
                  Orbit* _orbit,
                  const ReferenceFrame::SharedConstPtr& _bodyFrame,
                  RotationModel* _rotationModel,
                  FrameTree* _owner);

    TimelinePhase(const TimelinePhase& phase) = delete;
    TimelinePhase& operator=(const TimelinePhase& phase) = delete;

private:
    Body* m_body;

    double m_startTime;
    double m_endTime;

    ReferenceFrame::SharedConstPtr m_orbitFrame;
    Orbit* m_orbit;
    ReferenceFrame::SharedConstPtr m_bodyFrame;
    RotationModel* m_rotationModel;

    FrameTree* m_owner;
};

#endif // _CELENGINE_TIMELINEPHASE_H_
