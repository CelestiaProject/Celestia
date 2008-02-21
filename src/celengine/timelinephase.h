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

class ReferenceFrame;
class Orbit;
class RotationModel;
class FrameTree;
class Universe;
class Body;


class TimelinePhase
{
public:
    int addRef() const;
    int release() const;

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
        return m_orbitFrame;
    }

    Orbit* orbit() const
    {
        return m_orbit;
    }

    ReferenceFrame* bodyFrame() const
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

    static TimelinePhase* CreateTimelinePhase(Universe& universe,
                                              Body* body,
                                              double startTime,
                                              double endTime,
                                              ReferenceFrame& orbitFrame,
                                              Orbit& orbit,
                                              ReferenceFrame& bodyFrame,
                                              RotationModel& rotationModel);

private:
    // Private constructor; phases can only created with the
    // createTimelinePhase factory method.
    TimelinePhase(Body* _body,
                  double _startTime,
                  double _endTime,
                  ReferenceFrame* _orbitFrame,
                  Orbit* _orbit,
                  ReferenceFrame* _bodyFrame,
                  RotationModel* _rotationModel,
                  FrameTree* _owner);

    // Private copy constructor and assignment operator; should never be used.
    TimelinePhase(const TimelinePhase& phase);
    TimelinePhase& operator=(const TimelinePhase& phase);

    // TimelinePhases are refCounted; use release() instead.
    ~TimelinePhase();

private:
    Body* m_body;

    double m_startTime;
    double m_endTime;

    ReferenceFrame* m_orbitFrame;
    Orbit* m_orbit;
    ReferenceFrame* m_bodyFrame;
    RotationModel* m_rotationModel;

    FrameTree* m_owner;

    mutable int refCount;
};

#endif // _CELENGINE_TIMELINEPHASE_H_
