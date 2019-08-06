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

class ReferenceFrame;
class Orbit;
class RotationModel;
class FrameTree;
class Universe;
class Body;


class TimelinePhase
{
public:
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

    ReferenceFrame* orbitFrame() const
    {
        return m_orbitFrame.get();
    }

    Orbit* orbit() const
    {
        return m_orbit;
    }

    ReferenceFrame* bodyFrame() const
    {
        return m_bodyFrame.get();
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

    static std::shared_ptr<const TimelinePhase> CreateTimelinePhase(Universe& universe,
                                              Body* body,
                                              double startTime,
                                              double endTime,
                                              ReferenceFrame& orbitFrame,
                                              Orbit& orbit,
                                              ReferenceFrame& bodyFrame,
                                              RotationModel& rotationModel);

    ~TimelinePhase();

    // Private constructor; phases can only created with the
    // createTimelinePhase factory method. Made public for shared_ptr usage.
    TimelinePhase(Body* _body,
                  double _startTime,
                  double _endTime,
                  ReferenceFrame* _orbitFrame,
                  Orbit* _orbit,
                  ReferenceFrame* _bodyFrame,
                  RotationModel* _rotationModel,
                  FrameTree* _owner);

private:
    // Private copy constructor and assignment operator; should never be used.
    TimelinePhase(const TimelinePhase& phase);
    TimelinePhase& operator=(const TimelinePhase& phase);

private:
    Body* m_body;

    double m_startTime;
    double m_endTime;

    std::shared_ptr<ReferenceFrame> m_orbitFrame;
    Orbit* m_orbit;
    std::shared_ptr<ReferenceFrame> m_bodyFrame;
    RotationModel* m_rotationModel;

    FrameTree* m_owner;
};

#endif // _CELENGINE_TIMELINEPHASE_H_
