// timeline.h
//
// Object timelines.
//
// Copyright (C) 2008, the Celestia Development Team
// Initial version by Chris Laurel, claurel@gmail.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_TIMELINE_H_
#define _CELENGINE_TIMELINE_H_

#include <vector>

class ReferenceFrame;
class Orbit;
class RotationModel;
class TimelinePhase;

class Timeline
{
public:
    Timeline();
    ~Timeline();

    const TimelinePhase* findPhase(double t) const;
    bool appendPhase(TimelinePhase*);
    const TimelinePhase* getPhase(unsigned int n) const;
    unsigned int phaseCount() const;

    double startTime() const;
    double endTime() const;
    bool includes(double t) const;

    void markChanged();

private:
    std::vector<TimelinePhase*> phases;
};

#endif // _CELENGINE_TIMELINE_H_
